import assert from "node:assert/strict";
import { mkdir, mkdtemp, rm, writeFile } from "node:fs/promises";
import { tmpdir } from "node:os";
import path from "node:path";
import test from "node:test";
import { createServer } from "vite";

async function fixture(t) {
    const directory = await mkdtemp(path.join(tmpdir(), "split-flap-vite-"));
    t.after(() => rm(directory, { recursive: true, force: true }));
    const root = path.join(directory, "project");
    await mkdir(root);
    await writeFile(path.join(root, "package.json"), "{}");
    return { directory, root };
}

// https://github.com/advisories/GHSA-rq7h-c2jc-7f22
test(
    "an imported project path cannot authorize reading a host file outside server.fs.allow",
    { skip: process.platform === "win32" },
    async (t) => {
        const { directory, root } = await fixture(t);
        const outside = path.join(directory, "private.txt");
        await writeFile(outside, "PRIVATE HOST CONTENT\n");
        // Mirror the host path inside the project to populate safeModulePaths.
        const projectPath = outside.slice(1);
        const decoy = path.join(root, projectPath);
        await mkdir(path.dirname(decoy), { recursive: true });
        await writeFile(decoy, "PUBLIC PROJECT CONTENT\n");
        await writeFile(
            path.join(root, "main.js"),
            `import ${JSON.stringify(`./${projectPath}`)};`,
        );
        const vite = await createServer({
            configFile: false,
            root,
            logLevel: "silent",
            server: {
                host: "127.0.0.1",
                port: 0,
                hmr: false,
                fs: { allow: [root] },
            },
        });
        t.after(() => vite.close());
        await vite.listen();
        const base = `http://127.0.0.1:${vite.httpServer.address().port}`;
        const imported = await fetch(`${base}/main.js`);
        assert.equal(imported.status, 200);
        assert.ok((await imported.text()).includes(projectPath));
        const publicFile = await fetch(`${base}/@fs${decoy}`);
        assert.equal(publicFile.status, 200);
        assert.equal(await publicFile.text(), "PUBLIC PROJECT CONTENT\n");
        const response = await fetch(`${base}/@fs${outside}`);
        assert.equal(response.status, 403);
        assert.ok(!(await response.text()).includes("PRIVATE HOST CONTENT"));
    },
);

// https://github.com/advisories/GHSA-9jrq-w75r-8gcw
test("custom HTML transforms cannot inject a protocol-relative attacker script", async (t) => {
    const { root } = await fixture(t);
    const vite = await createServer({
        configFile: false,
        root,
        logLevel: "silent",
        appType: "custom",
        server: { middlewareMode: true, hmr: false, watch: null },
    });
    t.after(() => vite.close());
    const template =
        '<script type="module">document.body.textContent = "fixture"</script>';
    await writeFile(path.join(root, "index.html"), template);
    const normal = await vite.transformIndexHtml("/index.html", template);
    assert.match(normal, /src=["']\/index\.html\?html-proxy&index=0\.js["']/);
    for (const url of [
        "//attacker.invalid/payload.js?/../../package.json",
        "/index.html//attacker.invalid/payload.js?/../../package.json",
    ]) {
        const transformed = await vite.transformIndexHtml(url, template);
        const sources = [
            ...transformed.matchAll(/<script\b[^>]*\bsrc=["']([^"']+)["']/g),
        ].map((match) => match[1]);
        assert.ok(
            sources.length > 0,
            "Vite must inject its development client",
        );
        for (const source of sources) {
            assert.equal(
                new URL(source, "http://fixture.invalid").origin,
                "http://fixture.invalid",
                `Unexpected external script source for ${url}: ${source}`,
            );
        }
    }
});

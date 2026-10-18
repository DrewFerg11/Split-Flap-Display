import test from "node:test";
import assert from "node:assert/strict";
import { readFile } from "node:fs/promises";
import vm from "node:vm";

const source = await readFile(
    new URL("../../src/web/index.js", import.meta.url),
    "utf8",
);
async function page(
    fetchImpl = async () => ({
        json: async () => ({ type: "success", message: "Saved" }),
    }),
) {
    const factories = new Map(),
        requests = [],
        timers = [];
    let initialize;
    const Alpine = {
        data(name, factory) {
            factories.set(name, factory);
        },
        start() {
            initialize();
        },
    };
    const context = vm.createContext({
        window: { location: {} },
        document: {
            addEventListener(name, fn) {
                assert.equal(name, "alpine:init");
                initialize = fn;
            },
        },
        fetch: async (...args) => {
            requests.push(args);
            return fetchImpl(...args);
        },
        setTimeout(fn, delay) {
            timers.push({ fn, delay });
            return timers.length;
        },
        setInterval(fn, delay) {
            timers.push({ fn, delay });
            return timers.length;
        },
        clearInterval() {},
        confirm: () => true,
    });
    const module = new vm.SourceTextModule(source, { context });
    await module.link((name) => {
        assert.equal(name, "alpinejs");
        return new vm.SyntheticModule(
            ["default"],
            function () {
                this.setExport("default", Alpine);
            },
            { context },
        );
    });
    await module.evaluate();
    const model = factories.get("page")("Control");
    Object.assign(model.settings, {
        mode: 6,
        wireAddresses: "32,33,34,35",
        wireOffsets: "0,0,0,0",
    });
    return { model, requests, timers };
}
const settle = () => new Promise(setImmediate);
function payload(request) {
    return JSON.parse(request[1].body);
}

test("module edits keep addresses and offsets aligned", async () => {
    const { model: p } = await page();
    p.addWireModule();
    assert.equal(p.wireCount, 5);
    assert.equal(p.settings.wireOffsets, "0,0,0,0,0");
    p.setWireOffset(1, "-3");
    assert.equal(p.wireOffsetArray[1], "-3");
    p.removeWireModule();
    assert.equal(p.wireCount, 4);
    for (let i = 0; i < 10; i++) p.removeWireModule();
    assert.equal(p.wireCount, 1);
});
for (const multiple of [false, true])
    test(`single bus ${multiple ? "multiple" : "single"} request uses only /text`, async () => {
        const { model: p, requests } = await page();
        p.singleMode = !multiple;
        p.singleWord = "HELLO";
        p.multiWords = ["ONE", "TWO"];
        p.delay = 2;
        p.updateDisplay();
        await settle();
        assert.equal(requests.length, 1);
        assert.equal(requests[0][0], "/text");
        assert.equal(
            payload(requests[0]).mode,
            multiple ? "multiple" : "single",
        );
        assert.deepEqual(
            payload(requests[0]).words,
            multiple ? ["ONE", "TWO"] : ["HELLO"],
        );
    });
for (const multiple of [false, true])
    test(`dual row ${multiple ? "multiple" : "single"} request preserves rows`, async () => {
        const { model: p, requests } = await page();
        p.settings.wire1Addresses = "32,33";
        p.separateRows = true;
        p.singleMode = !multiple;
        p.row1 = "TOP";
        p.row2 = "BOTTOM";
        p.multiWords = ["TOP", "BOTTOM"];
        p.updateDisplay();
        await settle();
        const body = payload(requests[0]);
        assert.equal(body.mode, multiple ? "dual-multiple" : "dual-single");
        if (!multiple) {
            assert.equal(body.row1, "TOP");
            assert.equal(body.row2, "BOTTOM");
        }
        assert.equal(p.isWordOverLimit("ABC", 1), true);
        assert.equal(p.isWordOverLimit("ABC", 0), false);
    });
test("invalid input is rejected before fetch", async () => {
    const { model: p, requests } = await page();
    p.singleWord = "   ";
    p.updateDisplay();
    assert.equal(p.dialog.type, "error");
    p.singleMode = false;
    p.multiWords = [];
    p.updateDisplay();
    p.multiWords = ["A"];
    p.delay = 0;
    p.updateDisplay();
    assert.equal(requests.length, 0);
});
test("accuracy mode posts its own command and starts its timer", async () => {
    const { model: p, requests, timers } = await page();
    p.settings.mode = 9;
    p.accuracyDelay = 2;
    p.accuracyStepSize = 3;
    p.updateDisplay();
    await settle();
    assert.equal(requests.length, 1);
    assert.equal(requests[0][0], "/text");
    assert.deepEqual(payload(requests[0]), {
        mode: "accuracy",
        delay: 2,
        stepSize: 3,
    });
    assert.ok(timers.some((t) => t.delay === 2000));
    p.stopAccuracyTest();
    assert.equal(p.accuracyInterval, null);
});
test("date mode saves mode without issuing a competing text command", async () => {
    const { model: p, requests } = await page();
    p.settings.mode = 2;
    p.updateDisplay();
    await settle();
    assert.equal(requests.length, 1);
    assert.equal(requests[0][0], "/settings");
    assert.deepEqual(payload(requests[0]), { mode: 2 });
});
test("failed settings load clears loading and reports an error", async () => {
    const { model: p } = await page(async () => {
        throw new Error("offline");
    });
    p.loadSettings();
    await settle();
    assert.equal(p.loading.settings, false);
    assert.equal(p.dialog.type, "error");
});
test("save reports validation errors and releases saving state", async () => {
    const { model: p } = await page(async () => ({
        json: async () => ({
            type: "error",
            message: "Bad pin",
            errors: { key: "sdaPin", message: "Conflict" },
        }),
    }));
    p.save();
    assert.equal(p.saving, true);
    await settle();
    assert.equal(p.saving, false);
    assert.equal(p.errors.key, "sdaPin");
    assert.equal(p.errors.message, "Conflict");
    assert.equal(p.dialog.type, "error");
});
test("network failure posting text is visible", async () => {
    const { model: p } = await page(async () => {
        throw new Error("offline");
    });
    p.singleWord = "HI";
    p.updateDisplay();
    await settle();
    assert.equal(p.dialog.message, "offline");
    assert.equal(p.dialog.type, "error");
});

// The API rejects numeric strings. Keep form bindings consistent with that
// contract so editing an input does not turn a valid number into a string.
test("numeric settings inputs use Alpine numeric bindings", async () => {
    const html = await readFile(
        new URL("../../src/web/settings.html", import.meta.url),
        "utf8",
    );
    const inputs = [...html.matchAll(/<input\b[\s\S]*?>/g)]
        .map((match) => match[0])
        .filter(
            (input) =>
                /type="number"/.test(input) &&
                /x-model(?:\.number)?="settings\./.test(input),
        );
    assert.ok(inputs.length > 0, "numeric settings inputs were found");
    for (const input of inputs)
        assert.match(input, /x-model\.number="settings\./);
});

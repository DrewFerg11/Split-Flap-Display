# Dependency security checks

Run `npm ci` and `npm run test:security` with Node.js 24. The tests use Vite's
public APIs, temporary fixtures, and loopback connections. They do not launch an
editor, execute an attacker payload, or expose a development server externally.

The original lockfile's eight affected-package entries represented six distinct
advisories. Vite 6.4.5 includes the upstream fixes for all five Vite advisories
(the advisories identify 6.4.4 as the first patched 6.x release). Removing the
unused `vite-plugin-html` and `vite-plugin-static-copy` dependencies removes the
remaining affected dependency chain. The full updated npm audit reports zero
findings; this does not audit the ESP32 firmware's dependencies.

| Advisory | Project exposure and decision | Resolution and validation |
| --- | --- | --- |
| [GHSA-698v-4r3x-3vcv](https://github.com/advisories/GHSA-698v-4r3x-3vcv): Windows editor argument expansion | Affects developer tooling under Windows-specific filename/environment conditions. Worth patching for Windows contributors; not shipped in the display's static assets. | Upgrade Vite. Upstream fix verified by release range and bundled code review; Windows execution was not reproduced on Linux. |
| [GHSA-r825-w8mm-5pxw](https://github.com/advisories/GHSA-r825-w8mm-5pxw): editor CLI argument interpretation | For Vite, the documented exploit requires WSL, particular editors, and an attacker-created filename. Worth patching even though CI uses Linux. | Upgrade Vite. Upstream fix verified by release range and bundled code review; WSL/editor execution was not reproduced. |
| [GHSA-587h-7wmj-h5pm](https://github.com/advisories/GHSA-587h-7wmj-h5pm): Windows file-existence check bypass | Can combine with the Windows editor argument issue. Worth patching for the same developer-tooling boundary. | Upgrade Vite. Upstream fix verified by release range; Windows execution was not reproduced on Linux. |
| [GHSA-9jrq-w75r-8gcw](https://github.com/advisories/GHSA-9jrq-w75r-8gcw): custom middleware script injection | The checked-in Vite config does not use the advisory's custom middleware setup. Patch anyway to keep the underlying transform API safe for future development. | The regression test observes an attacker-controlled script source on 6.4.3 and blocks it on 6.4.5. It inspects transformed HTML without executing the script. |
| [GHSA-rq7h-c2jc-7f22](https://github.com/advisories/GHSA-rq7h-c2jc-7f22): files outside `server.fs.allow` | Requires a network-exposed non-Windows dev server and matching imported paths; the checked-in build does not configure that exposure. Patch the file-access boundary anyway. | The regression test reads a synthetic out-of-root file with 6.4.3 (HTTP 200), then verifies denial with 6.4.5 (HTTP 403). Skipped on Windows, which this advisory excludes. |
| [GHSA-g3hv-75wg-m249](https://github.com/advisories/GHSA-g3hv-75wg-m249): minimatch pattern amplification | Reported through `vite-plugin-html` → `ejs` → `jake`/`filelist` → `minimatch`. The plugin is unused, and the project does not provide untrusted glob patterns to that chain. Removal is preferable to forcing a transitive major upgrade. | Remove the unused plugins and their dependency trees. `npm ls` confirms these packages are absent; the asset build remains byte-for-byte identical. |

`@tailwindcss/vite`, `vite-plugin-html`, `vite-plugin-static-copy`, `ejs`, `jake`,
and `filelist` were affected-package entries through the dependency paths above,
rather than six additional independent advisories. The two regression tests were
both confirmed to fail with Vite 6.4.3 and pass with 6.4.5 on Linux.

GitHub's private Dependabot inventory was inaccessible during this review because
the cloud network policy blocked `api.github.com`. The table covers the complete
local npm audit inventory, rather than claiming exact correspondence with private
alert numbers or dismissal status.

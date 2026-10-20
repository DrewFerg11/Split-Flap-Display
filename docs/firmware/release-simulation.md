# ESP32 release simulation

This draft workflow integration implements [emulator MVP6/6 issue7](https://github.com/DrewFerg11/Split-Flap-Emulator/issues/7).
It retains the existing release environment approval, owner check, tag policy,
all three chip builds, and release/Pages publication steps. It adds two parallel
strict ESP32 simulations between builds and publication. No firmware runtime
code changes.

The helper is pinned to emulator commit `79e1f1aab1941ee8cc405e2b269c3c29d44384d5`.
[Full contract and reproduction](https://github.com/DrewFerg11/Split-Flap-Emulator/blob/79e1f1aab1941ee8cc405e2b269c3c29d44384d5/docs/release-gate.md)
describes input validation, artifact layout, bounded execution, strict version
matching, diagnostic retention and dry-run assertions.

Activation is blocked pending the actual verified published immutable image and
trusted hosted positive/negative evidence. There is no placeholder digest.
Configure the repository Actions variable `FLAPSIM_IMAGE` only from verified
phase5 publication proof, with the actual reference
`ghcr.io/drewferg11/flapsim@sha256:` plus its real64-character digest. Empty
configuration or a mutable tag fails closed. Keep this PR draft until actual
activation evidence exists.

The gate downloads this workflow run's `bins-esp32_n4` artifact and requires
`splitflap-esp32_n4-${GITHUB_REF_NAME}-factory.bin`; each layout has independent
flash/output. It uses strict installed smoke scenarios with the authorized tag
as `FIRMWARE_VERSION`, no golden version exception or known-issues baseline.
Existing firmware defects tracked in SFD54/84 and emulator14 can still prevent
the strict positive case. Baseline accommodation in emulator golden CI does not
establish a firmware fix or passing release acceptance.

Manually dispatch **Release / Tag** on an approved trusted version tag containing this
workflow with `dry_run=true` (manual default). Approval and all builds/tests still
run, but both Pages and GitHub Release publication are disabled. With
`sim_probe=none`, both layouts and publication eligibility must pass. With each
of `wrong-version`, `missing`, `corrupt`, and `timeout`, both matrix jobs and
publication eligibility intentionally fail; the independent assert-rejection
job must pass by checking the exact intended failure evidence from both layouts.
An unrelated failure cannot satisfy the rejection proof. Overall negative runs
remain red by design. Never push a test tag that could trigger production
publication merely to test a dry run; this integration creates no tags/releases.

Artifacts `release-sim-8` and `release-sim-16` retain `evidence/` plus runner CPU
specs for14 days on success/failure. They include run provenance, digest/tag/
checksum, reports/JUnit/replay, timeline, state/activity and serial/native/Docker
logs where available. Actions summaries report total matrix-job duration,
including pull and artifact handling. Record links for a positive dry run and
all four negative probes, exact image/helper pins and measured durations in
emulator issue7 before requesting production activation.

To update the emulator, verify/publish the new image first, review source/helper
diffs, update both the immutable image variable and reviewed helper commit,
repeat trusted dry-run proofs, and record old/new pins and evidence. Only the
maintainer merges or performs authorized production publication.

# ESP32 release simulation

This workflow integration implements [emulator MVP6/6 issue7](https://github.com/DrewFerg11/Split-Flap-Emulator/issues/7).
It retains the existing release environment approval, owner check, tag policy,
all three chip builds, and release/Pages publication steps. It adds two parallel
strict ESP32 simulations between builds and publication. No firmware runtime
code changes.

The helper is pinned to emulator release v0.2.0, commit `be1705c89858e8ee3974aa5bda33d3c393545b99`.
[Full contract and reproduction](https://github.com/DrewFerg11/Split-Flap-Emulator/blob/be1705c89858e8ee3974aa5bda33d3c393545b99/docs/release-gate.md)
describes input validation, artifact layout, bounded execution, strict version
matching, diagnostic retention and dry-run assertions.

The simulator image is emulator v0.2.0, pinned in the workflow by digest:
`ghcr.io/drewferg11/split-flap-emulator@sha256:aef570747499c185d819ce51ac9aeea37d5768584398d01b9df8c4c759866b63`.
That is the digest the emulator's own publication run recorded. Keeping it in the
workflow, not in a repository variable, means every change to it is reviewed. The
helper accepts only a digest; a mutable tag or an empty value fails closed.

The gate downloads this workflow run's `bins-esp32_n4` artifact and requires
`splitflap-esp32_n4-${GITHUB_REF_NAME}-factory.bin`; each layout has independent
flash/output. It uses strict installed smoke scenarios with the authorized tag
as `FIRMWARE_VERSION`, no golden version exception or known-issues baseline.
The settings race that failed the strict case on v1.4.x (#54/#84) is fixed in #91.
A hosted trial of this exact job against `main` at that fix (`ef65640`), with a
factory image built by these release steps, passed strictly 3/3 on each layout
(about 2 minutes of simulation each; [run](https://github.com/DrewFerg11/Split-Flap-Display/actions/runs/39792984149)).
The intermittent emulator I2C timeouts tracked in emulator #14 (fix in #90) can
still fail a run; a failure blocks publication and leaves the evidence below.

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
specs for 14 days on success/failure. They include run provenance, digest/tag/
checksum, reports/JUnit/replay, timeline, state/activity and serial/native/Docker
logs where available. Actions summaries report total matrix-job duration,
including pull and artifact handling. Record links for a positive dry run and
all four negative probes, exact image/helper pins and measured durations in
emulator issue7 before requesting production activation.

To update the emulator, verify/publish the new image first, review source/helper
diffs, update both the pinned image digest and the reviewed helper commit,
repeat trusted dry-run proofs, and record old/new pins and evidence. Only the
maintainer merges or performs authorized production publication.

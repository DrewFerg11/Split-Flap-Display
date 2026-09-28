# INJECT FIRMWARE_VERSION AND FIRMWARE_BUILD_SOURCE AT BUILD TIME
Import("env")

import os
import subprocess


def get_firmware_version():
    # In a release build, GITHUB_REF_NAME is the exact tag that triggered the
    # workflow (e.g. "v1.4.0") - the same deterministic value release.yml
    # already uses to name release assets. Prefer it over `git describe`,
    # which is ambiguous when a commit carries more than one exact-match tag
    # (e.g. a stable tag pushed at the same commit as an earlier RC tag, with
    # no new commits in between): its tie-break between candidates isn't
    # guaranteed to pick the newest one, so a "v1.4.0" build silently ended up
    # embedding "v1.4.0-rc.1" once. GITHUB_REF_TYPE == "tag" ensures this only
    # applies to actual tag builds, not e.g. push-to-main CI builds.
    if os.environ.get("GITHUB_ACTIONS") == "true" and os.environ.get("GITHUB_REF_TYPE") == "tag":
        ref_name = os.environ.get("GITHUB_REF_NAME")
        if ref_name:
            return ref_name

    try:
        return (
            subprocess.check_output(
                ["git", "describe", "--tags", "--always", "--dirty"],
                stderr=subprocess.DEVNULL,
            )
            .strip()
            .decode("utf-8")
        )
    except Exception:
        return "unknown"


def get_build_source():
    # GITHUB_ACTIONS is set to "true" on every GitHub Actions runner, which is
    # exactly release.yml's build environment - distinct from a developer
    # running `pio run` locally. Useful in boot logs to tell a pre-built
    # release binary apart from a local/dev build when debugging.
    if os.environ.get("GITHUB_ACTIONS") == "true":
        return "prebuilt (GitHub Actions)"
    return "local build"


version = get_firmware_version()
build_source = get_build_source()
print("VERSION: Firmware version resolved to " + version)
print("VERSION: Build source resolved to " + build_source)
env.Append(
    CPPDEFINES=[
        ("FIRMWARE_VERSION", '\\"%s\\"' % version),
        ("FIRMWARE_BUILD_SOURCE", '\\"%s\\"' % build_source),
    ]
)

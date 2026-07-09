# INJECT FIRMWARE_VERSION AND FIRMWARE_BUILD_SOURCE AT BUILD TIME
Import("env")

import os
import subprocess


def get_firmware_version():
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

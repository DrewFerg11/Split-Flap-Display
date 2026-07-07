# INJECT FIRMWARE_VERSION FROM GIT AT BUILD TIME
Import("env")

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


version = get_firmware_version()
print("VERSION: Firmware version resolved to " + version)
env.Append(CPPDEFINES=[("FIRMWARE_VERSION", '\\"%s\\"' % version)])

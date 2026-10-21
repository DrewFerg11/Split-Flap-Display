# INJECT FIRMWARE_VERSION AND FIRMWARE_BUILD_SOURCE AT BUILD TIME
Import("env")

import os
import re
import subprocess

# Release tags: vX.Y.Z, or a prerelease such as vX.Y.Z-rc.N / vX.Y.Z-beta.N.
TAG_PATTERN = re.compile(r"v(\d+)\.(\d+)\.(\d+)(?:-([0-9A-Za-z]+)(?:\.(\d+))?)?")


def git(*args):
    return (
        subprocess.check_output(["git", *args], stderr=subprocess.DEVNULL)
        .strip()
        .decode("utf-8")
    )


def tag_rank(tag):
    # A stable tag outranks any prerelease of the same version, so a commit
    # tagged both vX.Y.Z-rc.N and vX.Y.Z reports vX.Y.Z. Prerelease labels
    # order alphabetically, which gives alpha < beta < rc.
    match = TAG_PATTERN.fullmatch(tag)
    major, minor, patch, label, number = match.groups()
    stable = label is None
    return (int(major), int(minor), int(patch), stable, label or "", int(number or 0))


def get_firmware_version():
    # release.yml passes the tag it is releasing. Prefer it over git describe,
    # which picks arbitrarily among tags that point at the same commit.
    override = os.environ.get("FIRMWARE_VERSION_OVERRIDE", "").strip()
    if override:
        return override
    try:
        described = git("describe", "--tags", "--always", "--dirty")
    except Exception:
        return "unknown"
    try:
        tags = [
            t
            for t in git("tag", "--points-at", "HEAD").splitlines()
            if TAG_PATTERN.fullmatch(t)
        ]
    except Exception:
        tags = []
    suffix = "-dirty" if described.endswith("-dirty") else ""
    if tags and described[: len(described) - len(suffix)] in tags:
        return max(tags, key=tag_rank) + suffix
    return described


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

# FAIL CI IF THE ENV NAME LISTS DRIFT APART
#
# The env name for a board is written down in four places: platformio.ini
# (source of truth for what actually builds), ci-build.yml's PR matrix,
# release.yml's release matrix, and build/boards.json (the metadata the
# release scripts read). A prior drift incident let a board build in CI
# while being invisible to the release pipeline - see
# plans/ESP-BOARDS-PART-1-PLAN.md. This script is the guard against that
# recurring.
#
# Usage: check_boards_sync.py
# Exits non-zero (with a description) on any disagreement.

import json
import os
import re
import sys

ROOT = os.path.join(os.path.dirname(__file__), "..", "..")

NAMING_RULE = re.compile(r"^esp32([a-z]\d+)?_n\d+(r\d+)?(_ota)?$")


def read(path):
    with open(os.path.join(ROOT, path), encoding="utf-8") as f:
        return f.read()


def pio_envs():
    text = read("platformio.ini")
    names = re.findall(r"^\[env:([A-Za-z0-9_]+)\]", text, re.MULTILINE)
    names = [n for n in names if n != "ota"]
    build_envs = [n for n in names if not n.endswith("_ota")]
    ota_envs = [n for n in names if n.endswith("_ota")]
    return build_envs, ota_envs, names


def ci_build_envs():
    text = read(".github/workflows/ci-build.yml")
    m = re.search(r"env:\s*\[([^\]]+)\]", text)
    if not m:
        raise RuntimeError("could not find env matrix in ci-build.yml")
    return [e.strip() for e in m.group(1).split(",")]


def release_envs():
    text = read(".github/workflows/release.yml")
    return re.findall(r"^\s*-\s*env:\s*(\S+)\s*$", text, re.MULTILINE)


def boards_json():
    with open(os.path.join(ROOT, "build", "boards.json"), encoding="utf-8") as f:
        return json.load(f)


def main():
    errors = []

    build_envs, ota_envs, all_pio_envs = pio_envs()
    ci_envs = ci_build_envs()
    rel_envs = release_envs()
    boards = boards_json()
    boards_all = [b["env"] for b in boards]
    boards_released = [b["env"] for b in boards if b["released"]]

    if set(build_envs) != set(ci_envs):
        errors.append(
            "platformio.ini build envs %s != ci-build.yml matrix %s"
            % (sorted(build_envs), sorted(ci_envs))
        )

    if set(build_envs) != set(boards_all):
        errors.append(
            "platformio.ini build envs %s != build/boards.json envs %s"
            % (sorted(build_envs), sorted(boards_all))
        )

    if set(rel_envs) != set(boards_released):
        errors.append(
            "release.yml matrix envs %s != build/boards.json released envs %s"
            % (sorted(rel_envs), sorted(boards_released))
        )

    for name in all_pio_envs:
        if not NAMING_RULE.match(name):
            errors.append(
                "env '%s' does not match the naming rule esp32[<chip>]_n<MB>[r<MB>][_ota]"
                % name
            )

    if errors:
        print("check_boards_sync: FAILED")
        for e in errors:
            print(" - " + e)
        sys.exit(1)

    print("check_boards_sync: OK (%d board envs in sync)" % len(build_envs))


if __name__ == "__main__":
    main()

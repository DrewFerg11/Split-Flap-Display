# FAIL CI IF THE ENV NAME LISTS DRIFT APART
#
# The env name for a board is written down in four places: platformio.ini
# (source of truth for what actually builds), ci-build.yml's PR matrix,
# release.yml's release matrix, and build/boards.json (the metadata the
# release scripts read). A prior drift incident let a board build in CI
# while being invisible to the release pipeline - see
# plans/ESP-BOARDS-PART-1-PLAN.md. This script is the guard against that
# recurring. It also checks the per-board chip and bootloader offset, which
# release.yml's matrix duplicates from boards.json and which differ per chip.
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


def release_matrix():
    # Parse release.yml's build matrix into one dict per entry, e.g.
    # {"env": "esp32_n4", "chip": "esp32", "bootloader_offset": "0x1000"}.
    # Hand-rolled rather than pyyaml so this stays dependency-free like the
    # rest of the checks here.
    text = read(".github/workflows/release.yml")
    m = re.search(r"^(\s*)include:\s*$", text, re.MULTILINE)
    if not m:
        raise RuntimeError("could not find matrix include block in release.yml")
    indent = len(m.group(1))
    entries = []
    for line in text[m.end() :].splitlines():
        stripped = line.strip()
        if not stripped or stripped.startswith("#"):
            continue
        if len(line) - len(line.lstrip()) <= indent:
            break  # dedented back out of the include block
        if stripped.startswith("- "):
            entries.append({})
            stripped = stripped[2:]
        if not entries:
            continue
        kv = re.match(r"([A-Za-z0-9_]+):\s*(.*)$", stripped)
        if kv:
            entries[-1][kv.group(1)] = kv.group(2).strip().strip("\"'")
    return entries


def norm_offset(value):
    # Compare flash offsets numerically so "0x0" and "0x00" don't false-fail.
    try:
        return int(str(value), 16)
    except (TypeError, ValueError):
        return value


def boards_json():
    with open(os.path.join(ROOT, "build", "boards.json"), encoding="utf-8") as f:
        return json.load(f)


def main():
    errors = []

    build_envs, ota_envs, all_pio_envs = pio_envs()
    ci_envs = ci_build_envs()
    rel_matrix = release_matrix()
    rel_envs = [e["env"] for e in rel_matrix if "env" in e]
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

    # release.yml duplicates each board's chip and bootloader offset in its
    # matrix, and those two DO vary per board (0x1000 on ESP32, 0x0 on C3/S3).
    # Nothing else catches a mismatch: the merged factory image would be built
    # with the wrong bootloader offset and the board simply wouldn't boot.
    boards_by_env = {b["env"]: b for b in boards}
    for entry in rel_matrix:
        env = entry.get("env")
        board = boards_by_env.get(env)
        if board is None:
            continue  # unknown env is already reported by the set comparison above
        for yml_key, json_key, normalize in (
            ("chip", "chip", str),
            ("bootloader_offset", "bootloaderOffset", norm_offset),
        ):
            if yml_key not in entry:
                errors.append(
                    "release.yml matrix entry '%s' is missing '%s'" % (env, yml_key)
                )
            elif normalize(entry[yml_key]) != normalize(board.get(json_key)):
                errors.append(
                    "release.yml %s for env '%s' is '%s' but build/boards.json %s is '%s'"
                    % (yml_key, env, entry[yml_key], json_key, board.get(json_key))
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

# GENERATE ESP WEB TOOLS MANIFESTS FOR A RELEASE
#
# Produces two manifests as release assets:
#   manifest.json       - full factory flash (bootloader+partitions+otadata+app+littlefs), erases settings
#   manifest-app.json   - app-only update, preserves littlefs/settings
#
# Usage: generate_manifest.py <version> <download-base-url> <out-dir>

import json
import os
import sys

BOARDS_JSON = os.path.join(os.path.dirname(__file__), "..", "boards.json")


def load_boards():
    with open(BOARDS_JSON) as f:
        boards = json.load(f)
    return [b for b in boards if b["released"]]


def asset_name(env, version, kind):
    return "splitflap-%s-%s-%s.bin" % (env, version, kind)


def build_manifest(version, download_base_url, kind, offset):
    builds = []
    for board in load_boards():
        filename = asset_name(board["env"], version, kind)
        builds.append(
            {
                "chipFamily": board["chipFamily"],
                "parts": [
                    {"path": download_base_url.rstrip("/") + "/" + filename, "offset": offset}
                ],
            }
        )
    return {
        "name": "Split Flap Display",
        # Release tag (e.g. "v1.2.1"). ESP Web Tools shows this in the flash
        # dialog and compares it against the version Improv reports from the
        # running board, which is what powers the "already on this firmware"
        # detection the Update Firmware Only flow relies on.
        "version": version,
        # After a fresh factory install, ESP Web Tools reboots the board and
        # waits this many seconds for Improv to come up before showing the
        # Wi-Fi setup form. The board needs ~20s to finish booting to homing,
        # so the ESP Web Tools default of 10s is too short and the form gets
        # skipped - bump it to comfortably cover the boot.
        "new_install_improv_wait_time": 30,
        # With no Improv support yet (Step 4), ESP Web Tools can't detect the
        # running firmware, so new_install_prompt_erase=false makes it force a
        # full-chip erase + full write - a clean install with a single confirm
        # dialog. The factory image includes the bootloader, so this is safe.
        # (The app-only manifest is generated for Step 4 but not yet linked from
        # the install page: without Improv it would full-erase then write only
        # the app, leaving no bootloader.)
        "new_install_prompt_erase": False,
        "builds": builds,
    }


def main():
    if len(sys.argv) != 4:
        print("Usage: generate_manifest.py <version> <download-base-url> <out-dir>")
        sys.exit(1)

    version, download_base_url, out_dir = sys.argv[1], sys.argv[2], sys.argv[3]

    factory = build_manifest(version, download_base_url, "factory", offset=0)
    app = build_manifest(version, download_base_url, "app", offset=0x10000)

    with open(out_dir + "/manifest.json", "w") as f:
        json.dump(factory, f, indent=2)
    with open(out_dir + "/manifest-app.json", "w") as f:
        json.dump(app, f, indent=2)

    print("Generated manifest.json and manifest-app.json for " + version)


if __name__ == "__main__":
    main()

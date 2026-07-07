# GENERATE ESP WEB TOOLS MANIFESTS FOR A RELEASE
#
# Produces two manifests as release assets:
#   manifest.json       - full factory flash (bootloader+partitions+otadata+app+littlefs), erases settings
#   manifest-app.json   - app-only update, preserves littlefs/settings
#
# Usage: generate_manifest.py <version> <download-base-url> <out-dir>

import json
import sys

BOARDS = [
    {"env": "esp32_wroom", "chipFamily": "ESP32"},
    {"env": "esp32_c3", "chipFamily": "ESP32-C3"},
    {"env": "esp32_s3", "chipFamily": "ESP32-S3"},
]


def asset_name(env, version, kind):
    return "splitflap-%s-%s-%s.bin" % (env, version, kind)


def build_manifest(version, download_base_url, kind, offset):
    builds = []
    for board in BOARDS:
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
        "version": version,
        "new_install_prompt_erase": kind == "factory",
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

# RENDER THE RELEASE BODY FROM THE LOCKED TEMPLATE (see plans/RELEASE-AUTOMATION-PLAN.md)
#
# `generate_release_notes: true` in release.yml appends GitHub's own change list
# after this body - its own "## What's Changed" heading, the categorized PR
# bullets (grouped per .github/release.yml), and the "Full Changelog" link. This
# script renders only the fixed sections that precede it; we deliberately do NOT
# emit our own "### Changes" heading, since GitHub supplies "## What's Changed".
#
# Usage: render_release_notes.py <version> <prerelease: true|false> <meta-dir> <out-file> [ai-summary-file]
#
# <meta-dir> holds flash-<env>.txt files, one per board, each a single line:
#   <env>|<used-bytes>|<total-bytes>
# produced by the release workflow parsing the APPSIZE line from
# build/scripts/check_app_size.py: the on-disk firmware.bin size vs the app
# partition size. The .bin size (including MMU-alignment padding) is what the
# bootloader validates against the partition, so it is the honest figure -
# PlatformIO's smaller "Flash:" number is what let an oversized C3 image ship.
#
# [ai-summary-file], if given and non-empty, is a short AI-generated summary of the
# commit log (see release.yml's "AI release summary" step). Best-effort only - if
# the AI step failed or was skipped, the file is missing/empty and this section is
# omitted entirely. Never blocks a release.

import json
import os
import sys

INSTALL_PAGE_URL = "https://drewferg11.github.io/Split-Flap-Display/firmware/install-web-flasher/"
MANUAL_INSTALL_URL = "https://drewferg11.github.io/Split-Flap-Display/firmware/install-manual/"

DOCS_URL = "https://drewferg11.github.io/Split-Flap-Display/"
DISCORD_URL = "https://discord.gg/RCvks4XXXH"
ISSUES_URL = "https://github.com/DrewFerg11/Split-Flap-Display/issues"

BOARDS_JSON = os.path.join(os.path.dirname(__file__), "..", "boards.json")


def load_boards():
    with open(BOARDS_JSON) as f:
        boards = json.load(f)
    return [b for b in boards if b["released"]]


def install_section():
    return "\n".join(
        [
            "- \U0001f310 [Flash via web installer](%s)" % INSTALL_PAGE_URL,
            "- \U0001f6e0️ [Manual install instructions](%s)" % MANUAL_INSTALL_URL,
        ]
    )


def flash_table(meta_dir):
    rows = []
    for board in load_boards():
        env, label = board["env"], board["label"]
        path = os.path.join(meta_dir, "flash-%s.txt" % env)
        if not os.path.exists(path):
            continue
        with open(path) as f:
            parts = f.read().strip().split("|")
        if len(parts) != 3:
            continue
        try:
            used, total = int(parts[1]), int(parts[2])
        except ValueError:
            continue  # malformed/empty usage line - skip rather than fail the release
        if total <= 0:
            continue
        pct = used / total * 100
        rows.append("| %s | %d KB | %.1f%% |" % (label, used // 1024, pct))
    if not rows:
        return ""
    return (
        "\n### Flash usage\n\n"
        "| Board | Firmware size | % of app partition |\n"
        "|---|---|---|\n" + "\n".join(rows) + "\n"
    )


def boards_supported_section():
    return "\n".join("- %s" % board["label"] for board in load_boards())


def ai_summary_section(summary_file):
    if not summary_file or not os.path.exists(summary_file):
        return ""
    with open(summary_file, encoding="utf-8") as f:
        text = f.read().strip()
    if not text:
        return ""
    return "\n### Summary\n\n%s\n" % text


def main():
    if len(sys.argv) not in (5, 6):
        print(
            "Usage: render_release_notes.py <version> <prerelease> <meta-dir> "
            "<out-file> [ai-summary-file]"
        )
        sys.exit(1)

    version = sys.argv[1]
    prerelease = sys.argv[2] == "true"
    meta_dir = sys.argv[3]
    out_file = sys.argv[4]
    summary_file = sys.argv[5] if len(sys.argv) == 6 else None

    prerelease_note = (
        "\n> ⚠️ This is a **pre-release / release candidate** - expect rough edges.\n"
        if prerelease
        else ""
    )

    body = """## Split Flap Display {version}
{prerelease_note}
{summary}
### Boards supported
{boards_supported}

### Installation
{install_section}
{flash_table}
### Links
- \U0001f4d6 [Documentation]({docs})
- \U0001f4ac [Discord]({discord})
- \U0001f41b [Report an issue]({issues})

""".format(
        version=version,
        prerelease_note=prerelease_note,
        summary=ai_summary_section(summary_file),
        boards_supported=boards_supported_section(),
        install_section=install_section(),
        docs=DOCS_URL,
        flash_table=flash_table(meta_dir),
        discord=DISCORD_URL,
        issues=ISSUES_URL,
    )

    with open(out_file, "w", encoding="utf-8") as f:
        f.write(body)

    print("Rendered release notes for " + version)


if __name__ == "__main__":
    main()

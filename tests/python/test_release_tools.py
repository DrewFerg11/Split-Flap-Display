"""Exercise real release helpers without PlatformIO, git or network side effects."""

import contextlib
import importlib.util
import io
import json
import os
from pathlib import Path
import runpy
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
SCRIPTS = ROOT / "build" / "scripts"


def load(name):
    spec = importlib.util.spec_from_file_location(name, SCRIPTS / (name + ".py"))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class ReleaseManifestTest(unittest.TestCase):
    def setUp(self):
        self.manifest = load("generate_manifest")

    def test_factory_and_app_offsets_and_names(self):
        factory = self.manifest.build_manifest(
            "v1.4.1", "https://example.test/releases/", "factory", 0
        )
        app = self.manifest.build_manifest(
            "v1.4.1", "https://example.test/releases", "app", 0x10000
        )
        self.assertEqual(factory["version"], "v1.4.1")
        self.assertEqual(
            {b["chipFamily"] for b in factory["builds"]},
            {"ESP32", "ESP32-C3", "ESP32-S3"},
        )
        for image, suffix, offset in [(factory, "factory", 0), (app, "app", 0x10000)]:
            self.assertEqual(len(image["builds"]), 3)
            for build in image["builds"]:
                part = build["parts"][0]
                self.assertEqual(part["offset"], offset)
                self.assertTrue(part["path"].endswith("-v1.4.1-" + suffix + ".bin"))
                self.assertNotIn("releases//", part["path"])

    def test_unreleased_board_is_excluded(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "boards.json"
            path.write_text(
                json.dumps(
                    [
                        {"env": "ready", "chipFamily": "ESP32", "released": True},
                        {"env": "future", "released": False},
                    ]
                )
            )
            with patch.object(self.manifest, "BOARDS_JSON", str(path)):
                builds = self.manifest.build_manifest(
                    "v1.4.1-rc.1", "https://example.test", "app", 0x10000
                )["builds"]
            self.assertEqual(len(builds), 1)
            self.assertEqual(
                builds[0]["parts"][0]["path"],
                "https://example.test/splitflap-ready-v1.4.1-rc.1-app.bin",
            )

    def test_cli_writes_valid_json_files(self):
        with tempfile.TemporaryDirectory() as directory, patch(
            "sys.argv",
            ["generate_manifest.py", "v1.4.1", "https://example.test", directory],
        ), contextlib.redirect_stdout(io.StringIO()):
            self.manifest.main()
            for name, offset in [("manifest.json", 0), ("manifest-app.json", 0x10000)]:
                document = json.loads((Path(directory) / name).read_text())
                self.assertEqual(document["builds"][0]["parts"][0]["offset"], offset)


class BuildEnvironment:
    def __init__(self, directory, partition="partitions.csv"):
        self.directory = Path(directory)
        self.partition = partition
        self.definitions = {}

    def Append(self, **values):
        self.definitions.update(values)

    def AddPostAction(self, *args):
        pass

    def BoardConfig(self):
        return self

    def get(self, key, default=None):
        return self.partition

    def subst(self, value):
        return str(
            self.directory
            if value == "$PROJECT_DIR"
            else self.directory / "firmware.bin"
        )

    def Exit(self, code):
        raise SystemExit(code)


def build_script(name, env):
    with contextlib.redirect_stdout(io.StringIO()):
        return runpy.run_path(
            str(SCRIPTS / (name + ".py")),
            init_globals={"Import": lambda _: None, "env": env},
        )


class FirmwareMetadataTest(unittest.TestCase):
    def test_version_and_ci_source_are_injected(self):
        env = BuildEnvironment(".")
        with patch(
            "subprocess.check_output", return_value=b"v1.4.1-rc.1\n"
        ), patch.dict(os.environ, {"GITHUB_ACTIONS": "true"}):
            functions = build_script("inject_version", env)
            self.assertEqual(functions["get_firmware_version"](), "v1.4.1-rc.1")
            self.assertEqual(
                functions["get_build_source"](), "prebuilt (GitHub Actions)"
            )
        definitions = dict(env.definitions["CPPDEFINES"])
        self.assertIn("v1.4.1-rc.1", definitions["FIRMWARE_VERSION"])

    def run_git(self, describe, points_at, environ=None):
        def check_output(command, **_):
            if command[1] == "describe":
                return describe.encode() + b"\n"
            if command[1:3] == ["tag", "--points-at"]:
                return points_at.encode() + b"\n"
            raise AssertionError(command)

        environ = {"GITHUB_ACTIONS": "false", **(environ or {})}
        with patch("subprocess.check_output", side_effect=check_output), patch.dict(
            os.environ, environ
        ):
            if "FIRMWARE_VERSION_OVERRIDE" not in environ:
                os.environ.pop("FIRMWARE_VERSION_OVERRIDE", None)
            return build_script("inject_version", BuildEnvironment("."))[
                "get_firmware_version"
            ]()

    def test_stable_tag_wins_over_rc_on_the_same_commit(self):
        # v1.4.0 and v1.4.0-rc.1 both point at c0ff3dd; git describe picked the rc.
        self.assertEqual(self.run_git("v1.4.0-rc.1", "v1.4.0-rc.1\nv1.4.0"), "v1.4.0")
        self.assertEqual(
            self.run_git("v1.4.0-rc.1-dirty", "v1.4.0\nv1.4.0-rc.1"), "v1.4.0-dirty"
        )

    def test_highest_prerelease_wins_when_no_stable_tag(self):
        self.assertEqual(
            self.run_git("v1.5.0-rc.1", "v1.5.0-rc.1\nv1.5.0-rc.2"), "v1.5.0-rc.2"
        )

    def test_rc_outranks_beta_regardless_of_git_order(self):
        for listing in ["v1.5.0-beta.2\nv1.5.0-rc.2", "v1.5.0-rc.2\nv1.5.0-beta.2"]:
            self.assertEqual(self.run_git("v1.5.0-beta.2", listing), "v1.5.0-rc.2")

    def test_untagged_commits_keep_git_describe(self):
        self.assertEqual(self.run_git("v1.4.0-3-gabc1234", ""), "v1.4.0-3-gabc1234")
        self.assertEqual(self.run_git("abc1234", "nightly"), "abc1234")

    def test_release_workflow_override_wins(self):
        self.assertEqual(
            self.run_git(
                "v1.4.0-rc.1",
                "v1.4.0-rc.1\nv1.4.0",
                {"FIRMWARE_VERSION_OVERRIDE": "v1.4.0"},
            ),
            "v1.4.0",
        )
        self.assertEqual(
            self.run_git(
                "v1.4.0", "v1.4.0", {"FIRMWARE_VERSION_OVERRIDE": " v1.4.1-rc.2 "}
            ),
            "v1.4.1-rc.2",
        )

    def test_release_workflow_passes_the_tag_to_the_firmware_build(self):
        workflow = (ROOT / ".github" / "workflows" / "release.yml").read_text()
        self.assertIn("FIRMWARE_VERSION_OVERRIDE: ${{ github.ref_name }}", workflow)

    def test_version_failure_and_local_build_fall_back(self):
        with patch(
            "subprocess.check_output", side_effect=OSError("no git")
        ), patch.dict(os.environ, {"GITHUB_ACTIONS": "false"}):
            functions = build_script("inject_version", BuildEnvironment("."))
            self.assertEqual(functions["get_firmware_version"](), "unknown")
            self.assertEqual(functions["get_build_source"](), "local build")


class PartitionSizeTest(unittest.TestCase):
    def test_smallest_app_partition_limits_full_binary(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "partitions.csv").write_text(
                "# name,type,subtype,offset,size\napp0,app,ota_0,0x10000,0x100\napp1,app,ota_1,0x20000,0x80\nfs,data,spiffs,0x30000,0x400\n"
            )
            env = BuildEnvironment(directory)
            functions = build_script("check_app_size", env)
            self.assertEqual(functions["get_app_partition_size"]()[0], 128)
            with contextlib.redirect_stdout(io.StringIO()):
                for size in (127, 128):
                    (root / "firmware.bin").write_bytes(b"x" * size)
                    functions["check_bin_size"](None, None, env)
                (root / "firmware.bin").write_bytes(b"x" * 129)
                with self.assertRaises(SystemExit) as caught:
                    functions["check_bin_size"](None, None, env)
            self.assertEqual(caught.exception.code, 1)

    def test_no_app_partition_is_an_error(self):
        with tempfile.TemporaryDirectory() as directory:
            (Path(directory) / "partitions.csv").write_text(
                "fs,data,spiffs,0x30000,0x400\n"
            )
            functions = build_script("check_app_size", BuildEnvironment(directory))
            with self.assertRaisesRegex(RuntimeError, "No app partitions"):
                functions["get_app_partition_size"]()


if __name__ == "__main__":
    unittest.main()

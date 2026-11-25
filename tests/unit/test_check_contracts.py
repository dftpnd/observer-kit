import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]
CONTRACTS = ROOT / "contracts"
SCRIPT = ROOT / "scripts" / "check_contracts.py"


class ContractCliTests(unittest.TestCase):
    def setUp(self):
        build = ROOT / "build"
        build.mkdir(exist_ok=True)
        directory = tempfile.TemporaryDirectory(dir=build)
        self.addCleanup(directory.cleanup)
        self.directory = Path(directory.name)

    def cli(self, expected, *args):
        result = subprocess.run([sys.executable, SCRIPT, *args], capture_output=True, text=True)
        self.assertEqual(result.returncode, expected, result.stdout + result.stderr)
        self.assertEqual(result.stderr, "")
        return result.stdout

    def test_snapshots_and_fixtures(self):
        fixtures = ROOT / "tests/fixtures/contracts"
        for path in (fixtures / "valid").glob("*.json"):
            with self.subTest(path=path):
                self.assertEqual(path.read_bytes(), (CONTRACTS / path.name).read_bytes())
                self.cli(0, "check", path)
        for path in (fixtures / "invalid").glob("*.json"):
            with self.subTest(path=path):
                self.cli(1, "check", path)
        path = CONTRACTS / "offsets.client-6985.json"
        self.assertEqual(self.cli(0, "diff", path, fixtures / "valid" / path.name), "")
        self.assertIn("not-established", self.cli(2, self.directory / "missing.json"))

    def test_diff_reports_alias_changes_once(self):
        before = self.directory / "before.json"
        after = self.directory / "after.json"
        first, second = "dwEntityList", "dwGameEntitySystem"
        for left, right in [([first], [first, second]), ([first, second], [first]),
                            ([first], [second])]:
            with self.subTest(left=left, right=right):
                before.write_text(json.dumps({"schema_version": 1, "client.dll": dict.fromkeys(left, 1)}))
                after.write_text(json.dumps({"schema_version": 1, "client.dll": dict.fromkeys(right, 1)}))
                lines = self.cli(0, "diff", before, after).splitlines()
                self.assertEqual(len(lines), 1)
                self.assertTrue(lines[0].startswith("ALIAS-OK "))

    def test_malformed_inputs(self):
        path = self.directory / "data.json"
        path.write_text('{"schema_version":1,"client.dll":4}')
        self.cli(1, path)
        self.cli(1, "diff", path, path)
        info = CONTRACTS / "info.client-6985.json"
        self.cli(1, "diff", info, info)
        data = json.loads(info.read_text())
        data["renderer"] = "UPPERCASE"
        path.write_text(json.dumps(data))
        self.cli(1, path)
        path.write_text('{"schema_version":1,"schema_version":1,"sample.dll":{"X":1}}')
        self.cli(1, path)
        schema = self.directory / "schema.json"
        schema.write_text('{"$ref":"#/$defs/missing"}')
        self.cli(1, path, schema)
        schema.write_text('{"type":"number","maximum":1}')
        path.write_text("1.5")
        self.cli(1, path, schema)
        path.write_text("NaN")
        self.cli(1, path, schema)

    def test_manifest_validates_exact_paths(self):
        paths = sorted(CONTRACTS.glob("*.json"))
        manifest = self.directory / "MANIFEST.sha256"

        def entry(path):
            digest = hashlib.sha256(path.read_bytes()).hexdigest()
            return f"{digest}  {path.relative_to(ROOT)}\n"

        manifest.write_text("".join(entry(path) for path in paths))
        self.cli(0, "--manifest", manifest)
        self.cli(2, "--manifest", manifest, "extra")
        copy = self.directory / paths[0].name
        copy.write_text("{}")
        manifest.write_text("".join(entry(path) for path in [copy, *paths[1:]]))
        self.assertIn("файла нет в MANIFEST", self.cli(1, "--manifest", manifest))


if __name__ == "__main__":
    unittest.main()

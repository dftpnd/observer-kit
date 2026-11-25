import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
SAMPLE = ROOT / "tests/fixtures/host_env.sample.json"


class HostEnvTest(unittest.TestCase):
    def generate(self, value):
        build = ROOT / "build"
        build.mkdir(exist_ok=True)
        with tempfile.TemporaryDirectory(dir=build) as work:
            output = Path(work) / "host_env.hpp"
            command = [sys.executable, str(ROOT / "scripts/gen_host_env.py"), "--out", str(output)]
            if value is not None:
                env_file = Path(work) / "input.json"
                env_file.write_text(json.dumps(value), encoding="utf-8")
                command.extend(["--env", str(env_file)])
            environment = os.environ.copy()
            environment.pop("OVR_HOST_ENV", None)
            subprocess.run(command, env=environment, capture_output=True, check=True)
            return output.read_text(encoding="utf-8")

    def test_missing_or_malformed_produces_the_empty_header(self):
        sample = json.loads(SAMPLE.read_text(encoding="utf-8"))
        del sample["host"]["id"]
        empty = self.generate(None)
        self.assertIn("return false", empty)
        for value in ["invalid", sample, dict(sample, host="invalid")]:
            with self.subTest(value=value):
                self.assertEqual(self.generate(value), empty)

    def test_optional_handle(self):
        sample = json.loads(SAMPLE.read_text(encoding="utf-8"))
        del sample["handles"]["cv.particle_fow"]
        header = self.generate(sample)
        self.assertIn("return true", header)
        self.assertIn('"sample.bin"', header)
        self.assertIn('kCvParticleFow{""}', header)


if __name__ == "__main__":
    unittest.main()

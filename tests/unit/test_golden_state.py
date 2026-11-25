import hashlib
import json
from pathlib import Path
import subprocess
import sys


GOLDEN = Path(__file__).resolve().parents[1] / "golden"
FIXTURE = "synthetic-empty.json"
RAW = "raw/synthetic-empty.bin"


def verify_manifest():
    expected = {FIXTURE, RAW}
    for pattern in ("*.json", "raw/*.bin"):
        expected.update(path.relative_to(GOLDEN).as_posix() for path in GOLDEN.glob(pattern))
    listed = set()
    for line in (GOLDEN / "MANIFEST.sha256").read_text(encoding="utf-8").splitlines():
        digest, separator, name = line.partition("  ")
        if not separator or name not in expected or name in listed:
            raise ValueError(f"MANIFEST.sha256: {line!r}")
        actual = hashlib.sha256((GOLDEN / name).read_bytes()).hexdigest()
        if digest != actual:
            raise ValueError(f"field={name}.sha256 expected={digest} actual={actual}")
        listed.add(name)
    if listed != expected:
        raise ValueError("not-established: " + ", ".join(sorted(expected - listed)))


def compare(helper):
    verify_manifest()
    fixture = json.loads((GOLDEN / FIXTURE).read_text(encoding="utf-8"))
    if fixture["raw"] != Path(RAW).name or fixture["client_version"] != 0:
        raise ValueError(f"{FIXTURE}: raw/client_version")
    if (GOLDEN / RAW).stat().st_size != 0:
        raise ValueError(f"{RAW}: expected empty bytes")
    output = subprocess.check_output([helper, str(GOLDEN / RAW)], text=True)
    actual = json.loads(output)
    expected = fixture["state"]
    if actual.keys() != expected.keys():
        raise ValueError(f"field=state.keys expected={sorted(expected)} actual={sorted(actual)}")
    for field, value in expected.items():
        observed = actual[field]
        if field in ("actors", "agents"):
            if value != []:
                raise ValueError(f"field={field} expected=[] actual={value}")
            value = len(value)
        if type(value) is not type(observed) or value != observed:
            raise ValueError(f"field={field} expected={value!r} actual={observed!r}")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit("usage: test_golden_state.py <ovr_test_golden_state>")
    try:
        compare(sys.argv[1])
    except FileNotFoundError as error:
        sys.exit(f"not-established: {error.filename}; {FIXTURE}, {RAW}, MANIFEST.sha256")
    except (ValueError, KeyError, OSError, subprocess.CalledProcessError) as error:
        sys.exit(str(error))

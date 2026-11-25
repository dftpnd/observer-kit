"""Offline integration stub. No external data source is configured."""
import argparse

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    for flag in ("--dry-run", "--offline", "--selftest"):
        parser.add_argument(flag, action="store_true")
    parser.parse_args()
    print("not-established: offline mock; no version source")

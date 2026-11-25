"""Report availability of the environment audit in the offline build."""
import argparse

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--selftest", action="store_true")
    parser.add_argument("--env")
    parser.parse_args()
    print("not-established: offline mock; environment audit unavailable")

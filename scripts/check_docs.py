"""Check task IDs and dependencies in the execution plan."""
import argparse
from graphlib import CycleError, TopologicalSorter
from pathlib import Path
import re

def check(text):
    graph = {}
    for block in re.findall(r"^ID: (T\d{3}W?)\nMilestone: [^\n]*?Зависимости: ([^\n]+)", text, re.M):
        key, field = block
        if key in graph:
            raise ValueError(f"duplicate task {key}")
        graph[key] = re.findall(r"T\d{3}W?", field)
    headings = re.findall(r"^### (T\d{3}W?) —", text, re.M)
    if not graph or list(graph) != headings:
        raise ValueError("task headings and blocks differ")
    missing = {dep for deps in graph.values() for dep in deps} - graph.keys()
    if missing:
        raise ValueError(f"unknown dependencies: {sorted(missing)}")
    tuple(TopologicalSorter(graph).static_order())

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--selftest", action="store_true")
    args = parser.parse_args()
    path = Path(__file__).resolve().parents[1] / "TASKS.md"
    try:
        check(path.read_text(encoding="utf-8"))
        if args.selftest:
            try:
                check("### T000 — sample\nID: T000\nMilestone: M0. Зависимости: T000.")
            except CycleError:
                pass
            else:
                raise ValueError("cycle undetected")
    except ValueError as error:
        print(f"TASKS.md:1: {error}")
        raise SystemExit(1)

#!/usr/bin/env python3
"""Генератор ovr/host_env.hpp: имена среды хоста из HOST-ENV.json (PRINCIPLES P14).

Запуск:
  python3 scripts/gen_host_env.py [--env <HOST-ENV.json>] [--out <файл>]
Без --env файл берётся из OVR_HOST_ENV. Нет файла, файл битый или нет нужной ручки —
заглушка с пустыми значениями и is_available() == false (E1, P10), exit 0.
Пишет только в build/: настоящее имя хоста не попадает ни в stdout, ни в дерево.
"""

import argparse
import json
import os
import pathlib
import sys

DEFAULT_OUT = pathlib.Path("build") / "generated" / "ovr" / "host_env.hpp"

# константа заголовка -> путь к значению в HOST-ENV.json (ручка, не настоящее имя)
FIELDS = [
    ("kTarget", ("host", "id")),
    ("kPatchCurrent", ("handles", "patch_current")),
    ("kToolchain", ("sources", "src-A", "repo")),
    ("kHostBin", ("handles", "host_bin")),
    ("kHostBinLinux", ("handles", "host_bin_linux")),
    ("kVersionFile", ("handles", "version_file")),
    ("kCvCameraDistance", ("handles", "cv.camera_distance")),
    ("kCvRangeDisplay", ("handles", "cv.range_display")),
    ("kCvParticleFow", ("handles", "cv.particle_fow")),
]


def resolve(env, path):
    node = env
    for key in path:
        if isinstance(node, str):
            return node
        if not isinstance(node, dict) or key not in node:
            return ""
        node = node[key]
    return node if isinstance(node, str) else ""


def render(values):
    lines = [
        "// Сгенерировано scripts/gen_host_env.py: не править вручную (PRINCIPLES P14).",
        "#pragma once",
        "",
        "#include <string_view>",
        "",
        "namespace ovr::host_env {",
    ]
    for (name, _), value in zip(FIELDS, values):
        lines.append(f"inline constexpr std::string_view {name}{{{json.dumps(value)}}};")
    lines.append("inline bool is_available() { return %s; }" % ("true" if all(values) else "false"))
    lines += ["}", ""]
    return "\n".join(lines)


def main():
    ap = argparse.ArgumentParser(description="генерация host_env.hpp из HOST-ENV.json")
    ap.add_argument("--env", default="", help="файл среды хоста; по умолчанию OVR_HOST_ENV")
    ap.add_argument("--out", default=str(DEFAULT_OUT), help="куда писать заголовок")
    args = ap.parse_args()

    env_path = pathlib.Path(args.env or os.environ.get("OVR_HOST_ENV") or "")
    env = {}
    if env_path.is_file():
        try:
            env = json.loads(env_path.read_text(encoding="utf-8"))
        except (OSError, ValueError):
            print("gen_host_env: файл среды не разобран, сборка на заглушке", file=sys.stderr)
            env = {}

    values = [resolve(env, path) for _, path in FIELDS]
    text = render(values)

    out = pathlib.Path(args.out)
    out.parent.mkdir(parents=True, exist_ok=True)
    if not (out.is_file() and out.read_text(encoding="utf-8") == text):
        out.write_text(text, encoding="utf-8")
    return 0


if __name__ == "__main__":
    sys.exit(main())

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

# константа заголовка -> путь к значению в HOST-ENV.json (ручка, не настоящее имя).
# required=True: без этого среда хоста не опознаётся вообще (E1, fail-closed).
# required=False (ручки convar-ов): пустое значение = not-established для одного convar-а,
# среда при этом остаётся доступной (E23: часть convar-ов в текущей сборке уже удалена).
# kToolchain берётся из sources["src-A"]["repo"]: провенанс = источник, из которого мы
# собираем дампер, а не upstream того источника.
FIELDS = [
    ("kTarget", ("host", "id"), True),
    ("kPatchCurrent", ("handles", "patch_current"), True),
    ("kToolchain", ("sources", "src-A", "repo"), True),
    ("kHostBin", ("handles", "host_bin"), True),
    ("kHostBinLinux", ("handles", "host_bin_linux"), True),
    ("kVersionFile", ("handles", "version_file"), True),
    ("kCvCameraDistance", ("handles", "cv.camera_distance"), False),
    ("kCvRangeDisplay", ("handles", "cv.range_display"), False),
    ("kCvParticleFow", ("handles", "cv.particle_fow"), False),
]


def resolve(env, path):
    node = env
    for key in path:
        if not isinstance(node, dict) or key not in node:
            return ""
        node = node[key]
    if not isinstance(node, str) or any(not ch.isprintable() for ch in node):
        return ""
    return node


def render(values):
    lines = [
        "// Сгенерировано scripts/gen_host_env.py: не править вручную (PRINCIPLES P14).",
        "#pragma once",
        "",
        "#include <string_view>",
        "",
        "namespace ovr::host_env {",
    ]
    for (name, _, _), value in zip(FIELDS, values):
        lines.append(f"inline constexpr std::string_view {name}{{{json.dumps(value, ensure_ascii=False)}}};")
    # is_available() = только ручки идентичности: отсутствующий convar не глушит всю среду (E23).
    required = [value for (_, _, is_required), value in zip(FIELDS, values) if is_required]
    lines.append("inline bool is_available() { return %s; }" % ("true" if all(required) else "false"))
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

    values = [resolve(env, path) for _, path, _ in FIELDS]
    missing = [name for (name, _, _), value in zip(FIELDS, values) if not value]
    if missing:
        print("gen_host_env: не хватает %d из %d ручек: %s"
              % (len(missing), len(FIELDS), " ".join(missing)), file=sys.stderr)
    if any(required and not value for (_, _, required), value in zip(FIELDS, values)):
        print("gen_host_env: not-established", file=sys.stderr)
        values = [""] * len(FIELDS)
    text = render(values)

    out = pathlib.Path(args.out)
    out.parent.mkdir(parents=True, exist_ok=True)
    if not (out.is_file() and out.read_text(encoding="utf-8") == text):
        out.write_text(text, encoding="utf-8")
    return 0


if __name__ == "__main__":
    sys.exit(main())

#!/usr/bin/env python3
"""Проверка прямых #include по ARCHITECTURE.md §2, §4.2 и ограничениям T001."""
import argparse
from fnmatch import fnmatchcase
from pathlib import Path
import posixpath
import re
import tempfile

ROOT = Path(__file__).resolve().parents[1]
SUFFIXES = {".h", ".hh", ".hpp", ".hxx", ".c", ".cc", ".cpp", ".cxx", ".inc", ".inl", ".ipp"}
MODULES = {
    "core", "offsets", "memory", "state", "render", "overlay",
    "features", "input", "config", "ipc", "convars", "app",
}
CORE_HEADERS = {"array", "chrono", "cmath", "cstdint", "optional", "string_view"}

# §4.2. Имена заголовков нормализованы относительно include/ovr или src.
FORBIDDEN = {
    "state/*": ("overlay/*", "render/*", "windows.h", "*/windows.h"),
    "features/*": ("memory/*",),
    "render/world_to_screen.hpp": ("d3d*.h", "dxgi*.h", "directx*", "vulkan/*",
                                   "vulkan*.h", "vulkan*.hpp"),
}
# У нижних слоёв §2 закрытый список проектных зависимостей.
LAYER_IMPORTS = {
    "core": {"core"},
    "offsets": {"core", "offsets"},
    "memory": {"core", "offsets", "memory"},
}

# Лексемы, внутри которых маркеры препроцессора и числа принадлежат данным.
# Отдельная лексема для pp-number удерживает разделители разрядов в составе литерала.
LEXEMES = re.compile(
    r'(?P<comment>//(?:\\\n|[^\n])*|/\*.*?(?:\*/|\Z))'
    r'|(?P<raw>(?:u8|u|U|L)?R"(?P<delimiter>[^\s()\\]{0,16})\(.*?\)(?P=delimiter)")'
    r'|(?P<string>"(?:\\.|[^"\\\n])*")'
    r"|(?P<char>'(?:\\.|[^'\\\n])*')"
    r"|(?P<number>(?<!\w)(?:[0-9]|\.[0-9])(?:[eEpP][+-]|[\w.'])* )",
    re.DOTALL | re.VERBOSE,
)


def blank(text):
    return re.sub(r"[^\n]", " ", text)


def module_path(path):
    for prefix in ("include/ovr/", "src/", "ovr/"):
        if path.startswith(prefix):
            return path[len(prefix):]
    return path


def forbidden(source, header):
    module = source.partition("/")[0]
    other = header.partition("/")[0]
    if module in LAYER_IMPORTS and other in MODULES:
        return other not in LAYER_IMPORTS[module]
    if module == "core" and other not in MODULES:
        return header not in CORE_HEADERS and (source, header) != ("core/key.cpp", "string")
    return any(fnmatchcase(source, owner) and any(fnmatchcase(header.lower(), pat) for pat in banned)
               for owner, banned in FORBIDDEN.items())


def includes(path, text, root):
    def masked(token):
        if token["comment"] is not None or token["raw"] is not None:
            return blank(token[0])
        return token[0]

    # Сохраняем физические номера строк при склейке директив препроцессора.
    continuations = [m.start() for m in re.finditer(r"\\\n", text)]
    joined = text.replace("\\\n", "")
    code = LEXEMES.sub(masked, joined)
    source = module_path(path)
    for match in re.finditer(r'^[ \t]*#[ \t]*include[ \t]*([<"])([^">\n]+)[">]', code, re.MULTILINE):
        header = match[2].replace("\\", "/")
        sibling = root / posixpath.dirname(path) / header
        if header.startswith(".") or (match[1] == '"' and sibling.is_file()):
            header = posixpath.join(posixpath.dirname(path), header)
        header = module_path(posixpath.normpath(header))
        if forbidden(source, header):
            start = match.start()
            for position in continuations:
                if position > start:
                    break
                start += 2
            yield text.count("\n", 0, start) + 1, f"запрещённый #include {match[2]}"


def findings(root, check):
    for directory in ("src", "include"):
        for path in sorted((root / directory).rglob("*")):
            if path.suffix.lower() not in SUFFIXES or not path.is_file():
                continue
            relative = path.relative_to(root).as_posix()
            try:
                source = path.read_text(encoding="utf-8")
            except (UnicodeError, OSError) as error:
                yield f"{relative}:1: {error}"
                continue
            for line, message in check(relative, source, root):
                yield f"{relative}:{line}: {message}"


def command(check, cases):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=ROOT, help="корень исходников")
    parser.add_argument("--selftest", action="store_true")
    args = parser.parse_args()
    if args.selftest:
        base = ROOT / "build" / "selftest"
        base.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(dir=base) as temp:
            root = Path(temp)
            for filename, source, lines in cases:
                path = root / filename
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text(source, encoding="utf-8")
                actual = [msg.partition(": ")[0] for msg in findings(root, check)]
                path.unlink()
                if actual != [f"{filename}:{line}" for line in lines]:
                    print(f"{filename}:1: ожидались строки {lines}, получено {actual}")
                    return 1
        return 0
    problems = list(findings(args.root, check))
    for problem in problems:
        print(problem)
    return int(bool(problems))


if __name__ == "__main__":
    raise SystemExit(command(includes, [
        ("src/features/sample/check.cpp", '/* #include "memory/foo.hpp" */\n'
         '#include <ovr/state/host_state.hpp>\n# include "../../memory/fake.hpp"', [3]),
        ("include/ovr/state/check.hpp", '#include <Windows.h>\n#include "ovr/render/view.hpp"', [1, 2]),
        ("src/core/check.cpp", '#include "ovr/state/host_state.hpp"\n#include <array>', [1]),
        ("include/ovr/render/world_to_screen.hpp", '#include <vulkan/vulkan.h>', [1]),
    ]))

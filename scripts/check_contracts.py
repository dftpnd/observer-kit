#!/usr/bin/env python3
"""Валидатор контрактов contracts/*.json по JSON-схемам (stdlib, без зависимостей).

Запуск:
  python3 scripts/check_contracts.py <файл> [<схема>]         проверка снапшота
  python3 scripts/check_contracts.py diff <файл_a> <файл_b>   added/removed/changed + ALIAS-OK
  python3 scripts/check_contracts.py --manifest [<файл>]      сверка contracts/MANIFEST.sha256
Схема по умолчанию выбирается по форме данных: есть ключ target -> info, иначе offsets.
Имена в MANIFEST разрешаются относительно каталога манифеста, потом относительно корня.
exit 0 = ок, exit 1 = нарушение контракта (E2), exit 2 = нет файла или аргументов (E3).
"""

import hashlib
import json
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
SCHEMA_DIR = ROOT / "contracts" / "schemas"
OFFSETS_SCHEMA = SCHEMA_DIR / "offsets.schema.json"
INFO_SCHEMA = SCHEMA_DIR / "info.schema.json"
DEFAULT_MANIFEST = ROOT / "contracts" / "MANIFEST.sha256"
DEFAULT_REQUIRED = ROOT / "scripts" / "required_symbols.txt"

# Пары-алиасы (contracts/README.md:37): равны всегда, расхождением не считаются.
ALIASES = [
    (("client.dll", "dwEntityList"), ("client.dll", "dwGameEntitySystem")),
    (("engine2.dll", "dwNetworkGameClient_deltaTick"),
     ("engine2.dll", "dwNetworkGameClient_serverTickCount")),
]

TYPES = {
    "object": lambda v: isinstance(v, dict),
    "array": lambda v: isinstance(v, list),
    "string": lambda v: isinstance(v, str),
    "integer": lambda v: isinstance(v, int) and not isinstance(v, bool),
    "number": lambda v: isinstance(v, (int, float)) and not isinstance(v, bool),
    "boolean": lambda v: isinstance(v, bool),
    "null": lambda v: v is None,
}


def rel(path):
    try:
        return str(pathlib.Path(path).resolve().relative_to(ROOT))
    except ValueError:
        return str(path)


def load(path):
    return json.loads(pathlib.Path(path).read_text(encoding="utf-8"))


def pointer_lookup(root, ref):
    node = root
    for raw in [part for part in ref.lstrip("#").split("/") if part]:
        part = raw.replace("~1", "/").replace("~0", "~")
        if isinstance(node, dict) and part in node:
            node = node[part]
        else:
            return {}
    return node


def deref(root, node):
    for _ in range(8):
        if isinstance(node, dict) and "$ref" in node:
            node = pointer_lookup(root, node["$ref"])
            continue
        return node
    return {}


def validate(value, schema, root, pointer, errors):
    schema = deref(root, schema)
    if not isinstance(schema, dict):
        return

    want = schema.get("type")
    if want is not None:
        names = want if isinstance(want, list) else [want]
        if not any(TYPES.get(name, lambda _: False)(value) for name in names):
            errors.append((pointer, "ожидался тип " + " | ".join(names)))
            return

    if "const" in schema and value != schema["const"]:
        errors.append((pointer, f"ожидалось {schema['const']!r}"))
    if "enum" in schema and value not in schema["enum"]:
        errors.append((pointer, f"значение вне перечня {schema['enum']}"))

    if isinstance(value, int) and not isinstance(value, bool):
        if "minimum" in schema and value < schema["minimum"]:
            errors.append((pointer, f"меньше {schema['minimum']}"))
        if "maximum" in schema and value > schema["maximum"]:
            errors.append((pointer, f"больше {schema['maximum']}"))

    if isinstance(value, str):
        pattern = schema.get("pattern")
        if pattern and not re.search(pattern, value):
            errors.append((pointer, f"не совпадает с шаблоном {pattern}"))
        if "minLength" in schema and len(value) < schema["minLength"]:
            errors.append((pointer, f"короче {schema['minLength']}"))

    if isinstance(value, dict):
        for key in schema.get("required", []):
            if key not in value:
                errors.append((pointer, f"нет обязательного ключа {key}"))
        if "minProperties" in schema and len(value) < schema["minProperties"]:
            errors.append((pointer, f"ключей меньше, чем {schema['minProperties']}"))

        name_pattern = schema.get("propertyNames", {}).get("pattern")
        props = schema.get("properties", {})
        patterns = list(schema.get("patternProperties", {}).items())
        additional = schema.get("additionalProperties", True)
        for key, sub in value.items():
            if name_pattern and not re.search(name_pattern, key):
                errors.append((pointer, f"имя ключа {key} не из схемы"))
                continue
            matched = False
            if key in props:
                matched = True
                validate(sub, props[key], root, f"{pointer}/{key}", errors)
            for raw, sub_schema in patterns:
                if re.search(raw, key) and not matched:
                    matched = True
                    validate(sub, sub_schema, root, f"{pointer}/{key}", errors)
            if matched:
                continue
            if additional is False:
                errors.append((pointer, f"ключ {key} не описан схемой"))
            elif isinstance(additional, dict):
                validate(sub, additional, root, f"{pointer}/{key}", errors)

    if isinstance(value, list):
        if "minItems" in schema and len(value) < schema["minItems"]:
            errors.append((pointer, f"элементов меньше, чем {schema['minItems']}"))
        if schema.get("uniqueItems"):
            dumped = [json.dumps(item, sort_keys=True) for item in value]
            if len(set(dumped)) != len(dumped):
                errors.append((pointer, "элементы не уникальны"))
        items = schema.get("items")
        if isinstance(items, dict):
            for index, item in enumerate(value):
                validate(item, items, root, f"{pointer}/{index}", errors)


def line_of(path, key):
    if not key or not path.is_file():
        return 1
    rx = re.compile('"' + re.escape(key) + '"\\s*:')
    for number, text in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        if rx.search(text):
            return number
    return 1


def show(path, pointer, message):
    segments = [part for part in pointer.split("/") if part and not part.isdigit()]
    key = segments[-1] if segments else ""
    print(f"{rel(path)}:{line_of(path, key)}: {pointer or '.'}: {message}")


def not_established(what, need):
    print(f"not-established: нет {what} — надо добыть: {'; '.join(need)}")


def infer_schema(data):
    return INFO_SCHEMA if isinstance(data, dict) and "target" in data else OFFSETS_SCHEMA


def offset_symbols(data):
    for module, symbols in data.items():
        if module.endswith(".dll") and isinstance(symbols, dict):
            for name, value in symbols.items():
                yield module, name, value


def extra_rules(data, errors):
    for module, name, value in offset_symbols(data):
        if isinstance(value, int) and not isinstance(value, bool) and value == 0:
            errors.append((f"/{module}/{name}", "смещение равно 0 — значение не прочитано"))
    if DEFAULT_REQUIRED.is_file():
        names = [line.strip() for line in
                 DEFAULT_REQUIRED.read_text(encoding="utf-8").splitlines()
                 if line.strip() and not line.startswith("#")]
        for name in names:
            module, _, symbol = name.partition("!")
            if not module or not symbol:
                continue
            if module not in data:
                errors.append(("", f"нет модуля {module} из перечня обязательных символов"))
            elif symbol not in data.get(module, {}):
                errors.append((f"/{module}", f"нет обязательного символа {name}"))
    for left, right in ALIASES:
        first = data.get(left[0], {}).get(left[1])
        second = data.get(right[0], {}).get(right[1])
        if first is not None and second is not None and first != second:
            errors.append((f"/{left[0]}", f"алиасы разошлись: {left[1]} != {right[1]}"))


def check_file(path, schema_path=None):
    path = pathlib.Path(path)
    if not path.is_file():
        not_established(rel(path), ["сам снапшот (запуск дампера, T011)",
                                    "копии в tests/fixtures/contracts/valid/"])
        return 2
    try:
        data = load(path)
    except (OSError, ValueError) as error:
        print(f"{rel(path)}:1: .: файл не разобран: {error}")
        return 1
    schema_path = pathlib.Path(schema_path) if schema_path else infer_schema(data)
    if not schema_path.is_file():
        not_established(rel(schema_path), ["схема контракта в contracts/schemas/ (владелец T002)"])
        return 2
    try:
        schema = load(schema_path)
    except (OSError, ValueError) as error:
        print(f"{rel(schema_path)}:1: .: схема не разобрана: {error}")
        return 1

    errors = []
    validate(data, schema, schema, "", errors)
    if isinstance(data, dict) and any(key.endswith(".dll") for key in data):
        extra_rules(data, errors)
    for pointer, message in errors:
        show(path, pointer, message)
    if errors:
        return 1
    print(f"{rel(path)}: ok ({rel(schema_path)})")
    return 0


def alias_twin(module, name):
    for left, right in ALIASES:
        if (module, name) == left:
            return right
        if (module, name) == right:
            return left
    return None


def cmd_diff(left_text, right_text):
    paths = [pathlib.Path(left_text), pathlib.Path(right_text)]
    for path in paths:
        if not path.is_file():
            not_established(rel(path), ["снапшот смещений (запуск дампера, T011)"])
            return 2
    data = []
    for path in paths:
        try:
            payload = load(path)
        except (OSError, ValueError) as error:
            print(f"{rel(path)}:1: .: файл не разобран: {error}")
            return 1
        data.append(payload)

    if check_file(left_text) == 1 or check_file(right_text) == 1:
        return 1

    before = {(module, name): value for module, name, value in offset_symbols(data[0])}
    after = {(module, name): value for module, name, value in offset_symbols(data[1])}
    for key in sorted(set(before) - set(after)):
        twin = alias_twin(*key)
        if twin is not None and after.get(twin) == before[key]:
            print(f"ALIAS-OK {key[0]}!{key[1]} == {twin[0]}!{twin[1]}")
        else:
            print(f"removed: {key[0]}!{key[1]}")
    for key in sorted(set(after) - set(before)):
        print(f"added: {key[0]}!{key[1]} = {after[key]}")
    for key in sorted(set(after) & set(before)):
        if after[key] == before[key]:
            continue
        twin = alias_twin(*key)
        if twin is not None and after.get(twin) == before[key]:
            print(f"ALIAS-OK {key[0]}!{key[1]} == {twin[0]}!{twin[1]}")
        else:
            print(f"changed: {key[0]}!{key[1]} {before[key]} -> {after[key]}")
    return 0


def cmd_manifest(manifest):
    manifest = pathlib.Path(manifest)
    if not manifest.is_file():
        not_established(rel(manifest),
                        ["SHA-256 каждого закоммиченного снапшота (P8, владелец T011)"])
        return 2
    failures = 0
    listed = set()
    for number, line in enumerate(manifest.read_text(encoding="utf-8").splitlines(), 1):
        digest, _, name = line.partition("  ")
        name = name.strip()
        if not digest or not name:
            print(f"{rel(manifest)}:{number}: .: строка не в формате <sha256>  <имя файла>")
            failures += 1
            continue
        listed.add(pathlib.Path(name).name)
        target = manifest.parent / name
        if not target.is_file():
            target = ROOT / name
        if not target.is_file():
            print(f"{rel(manifest)}:{number}: {name}: файла нет в дереве")
            failures += 1
            continue
        actual = hashlib.sha256(target.read_bytes()).hexdigest()
        if actual != digest.strip():
            print(f"{rel(manifest)}:{number}: {name}: SHA-256 разошёлся")
            failures += 1
    for path in sorted((ROOT / "contracts").glob("*.json")):
        if path.name not in listed:
            print(f"{rel(manifest)}:1: {path.name}: файла нет в MANIFEST")
            failures += 1
    if failures:
        return 1
    print(f"{rel(manifest)}: ok ({len(listed)} файлов)")
    return 0


def main(argv):
    if len(argv) == 3 and argv[0] == "diff":
        return cmd_diff(argv[1], argv[2])
    if argv and argv[0] == "--manifest":
        return cmd_manifest(argv[1] if len(argv) > 1 else DEFAULT_MANIFEST)
    if len(argv) == 1:
        return check_file(argv[0])
    if len(argv) == 2:
        return check_file(argv[0], argv[1])
    print(__doc__)
    return 2


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))

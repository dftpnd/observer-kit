#!/usr/bin/env python3
"""Целочисленные литералы смещений в src/ и include/ (ARCHITECTURE.md §8.1)."""
import hashlib
import re
import sys

sys.dont_write_bytecode = True
from check_includes import LEXEMES, blank, command

LIMIT = 100000
INTEGER = re.compile(r"(0[xX][0-9a-fA-F]+|0[bB][01]+|0[0-7]*|[1-9][0-9]*)[uUlLzZ]*")

# Только эти две последовательности 32-битных слов принадлежат SHA-256.
SHA256_TABLES = {
    "kSeed": "5cce668898a2e2d7dedbb552165a2a77596d092b03df3fcf68118d63982b6106",
    "kRoundConstants": "e4c62ae41929873b99fa3f871694451c9dbd0300448aba7ea4ff6d68a6b52d95",
}
TABLE = re.compile(r"\b(kSeed|kRoundConstants)\s*=\s*\{((?:\s*0x[0-9a-fA-F]{8}u\s*,)+\s*)\}")


def mask_sha_table(match):
    words = re.findall(r"0x([0-9a-fA-F]{8})u", match[2])
    digest = hashlib.sha256(bytes.fromhex("".join(words))).hexdigest()
    return blank(match[0]) if digest == SHA256_TABLES[match[1]] else match[0]


def literals(path, text, _root):
    if path == "src/offsets/offset_registry.cpp":
        text = TABLE.sub(mask_sha_table, text)
    newline_pos = 0
    line = 1
    for token in LEXEMES.finditer(text):
        line += text.count("\n", newline_pos, token.start())
        newline_pos = token.start()
        if token["number"] is None:
            continue
        match = INTEGER.fullmatch(token[0].replace("'", ""))
        if match is None:
            continue
        digits = match[1].lower()
        base = 8 if digits.startswith("0") and not digits.startswith(("0x", "0b")) else 0
        if int(digits, base) >= LIMIT:
            yield line, f"литерал {token[0]} требует проверки по §8.1"


if __name__ == "__main__":
    raise SystemExit(command(literals, [
        ("src/check.cpp", "int a = 99999;\nint b = 698512;", [2]),
        ("include/check.hpp", "int a = 0x186A0U;\nint b = 100'000;\n"
         "int c = 0303240;\nint d = 0b11000011010100000;", [1, 2, 3, 4]),
        ("src/check.cpp", 'auto a = R"(100000)"; // 100000\n/* 345789 */ float f = 1e6f;', []),
        ("build/check.cpp", "int n = 698512;", []),
        ("tests/unit/check.cpp", "int n = 698512;", []),
    ]))

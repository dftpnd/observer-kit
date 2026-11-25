// Скан байтовых образцов (ARCHITECTURE.md §10.9, T010): инструмент дампера (T011),
// в рантайме наблюдателя не участвует. Вызывать его имеет право только tools/dumper/**.
#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace ovr::offsets::sigscan {
struct Pattern { std::string_view mnemonic; std::string_view text; };
struct Match { std::uintptr_t address; std::uint32_t size; };

// Образ — байты дампа целиком; base = адрес первого байта образа, адрес совпадения = base + индекс.
// Pattern.text: «48 8B 05 ?? ?? ?? ??» — байты через пробел, `??` любой байт. Половину байта
// задают две формы: `?X` фиксирует младший ниббл (старший любой), `X?` фиксирует старший.
// Разбор толерантный (повторные пробелы, регистр hex); невалидный шаблон или 0 совпадений дают
// пустой результат и ровно одну строку в stderr по этому шаблону (E39). Исключений нет.
std::vector<Match> scan(std::span<const std::byte> image, std::span<const Pattern> patterns);

// Цель rip-относительного адреса: base + address + instruction_length + int32 из disp_offset.
// Любая граница нарушена (disp32 не читается whole-in-image, цель вышла за образ) -> nullopt,
// неопределённого поведения нет (E40).
std::optional<std::uintptr_t> rip_target(std::span<const std::byte> image, const Match& match,
                                         std::size_t disp_offset, std::size_t instruction_length);
}

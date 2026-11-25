#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace ovr::memory {

struct MapEntry {
    std::string module;
    std::uintptr_t base = 0;
    std::vector<std::uint8_t> bytes;
};

struct DumpLoadResult {
    std::vector<MapEntry> entries;
    bool ok = false;
    std::string error;   // пустой, когда ok == true
};

// Читает карту дампа [{module, base, file, file_offset, size}] и байты по ссылкам на файл.
// Разбор — частный случай JSON (массив объектов с этими пятью ключами): своего парсера в
// проекте нет, third_party появляется только в T014. Файл дампа открывается один раз здесь,
// внутри чтения пачек он не открывается повторно (E7).
DumpLoadResult load_dump_map(const std::string& map_path);

}

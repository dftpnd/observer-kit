// Реестр смещений (ARCHITECTURE.md §3.3): единственный вход в адреса хоста.
// Числа приходят только из contracts/*.json; семантика разыменования — RESEARCH.md §5.
#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>

namespace ovr::offsets {
enum class Deref : std::uint8_t { None, Once, Chain };   // семантика: RESEARCH.md §5
struct Symbol { std::uintptr_t rva; Deref deref; std::uint16_t stride; };

// Копируемый и неизменяемый после load_from: изменённый снапшот = новый реестр, а не правка.
class OffsetRegistry {
public:
    // Только fail-closed: бросать запрещено. Любая проблема (нет файла, битый JSON, неизвестный
    // ключ схемы) -> пустой реестр (build() == 0, sha256() пустой, lookup() == nullopt) и одна
    // строка в stderr с причиной (E2, E30). Файл ищется по имени offsets.client-<build>.json.
    static OffsetRegistry load_from(const std::string& contracts_dir, int build);

    // Ключ "<module>!<symbol>" ровно как в RESEARCH.md §2; регистр модуля как в JSON.
    std::optional<Symbol> lookup(std::string_view name) const;

    int build() const;
    std::string_view sha256() const;

    // Триггер re-dump (риск R3): чужой билд хоста. current_build == 0 = билд неизвестен -> не устарел.
    bool is_stale(int current_build) const;

private:
    int build_ = 0;
    std::string sha256_hex_;
    // Прозрачный компаратор: lookup() по string_view не аллоцирует (бюджет кадра, §5).
    std::map<std::string, Symbol, std::less<>> symbols_;
};
}

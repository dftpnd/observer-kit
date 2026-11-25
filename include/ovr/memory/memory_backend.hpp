#pragma once

#include <cstdint>
#include <span>
#include <string_view>

namespace ovr::memory {
struct ReadResult { const std::uint8_t* data{}; std::uint32_t size{}; bool ok{}; };
struct ReadReq { std::uintptr_t addr; void* dst; std::uint32_t size; ReadResult* result{}; };

// Единая точка входа в чужую память: реализаций три (T005, T006, T006W), и ни одна из них
// не знает ни про смещения, ни про фичи. Здесь нет ни слова про ОС и про хоста:
// ОС-специфика живёт только в реализациях. Запись вынесена в IMemoryWriter (§10.6) —
// всё остальное read-only (риск R2).
class IMemoryBackend {
public:
    virtual ~IMemoryBackend() = default;

    // false, если процесса нет или читать его нечем.
    virtual bool attach(std::uint32_t pid) = 0;

    // 0, если модуль не найден.
    virtual std::uintptr_t module_base(std::string_view module) const = 0;

    // false, если прочитано меньше байт, чем запрошено; при провале dst остаётся нетронутым,
    // исключений нет (E6, E7). size == 0 — вырожденный запрос: читать нечего, ответ false
    // (реализации T005/T006/T006W обязаны отвечать одинаково; в §3.2 этот случай не описан).
    virtual bool read_raw(std::uintptr_t addr, void* dst, std::uint32_t size) = 0;

    // result указывает на отдельный выходной объект вызывающего либо равен nullptr.
    // При успехе результат содержит dst, size и true. При отказе все поля сбрасываются.
    // Одна пачка на кадр (бюджет §5), а не сотни отдельных вызовов. Исключений не бросает:
    // провал одного диапазона не роняет остальные — каждый запрос отвечает своим ok == false,
    // и именно флаги рассказывают разбору о частичном отказе (E6). Пачка длиннее лимита ниже
    // не аллоцируется и не является неопределённым поведением: лишние диапазоны получают
    // ok == false (E48).
    virtual void read_batch(std::span<const ReadReq> reqs) = 0;
};

constexpr std::uint32_t kMaxReadRequests = 256;   // §10.15; пачка длиннее — лишние диапазоны ok == false, без аллокаций и UB (E48); больше 256 — находка ревью
}

#pragma once

#include <cstdint>
#include <span>

namespace ovr::memory {
struct WriteReq { std::uintptr_t addr; const void* src; std::uint32_t size; };
struct WriteResult { std::uint32_t written; bool ok; };

// Единственный канал изменения хоста (риск R2): им пользуются только convar-ы (T022) и
// дампер (T011). Буфер src принадлежит вызывающему: аллокаций внутри нет.
class IMemoryWriter {
public:
    virtual ~IMemoryWriter() = default;

    // false, если записано меньше байт, чем запрошено; при провале буфер не трогается.
    // size == 0 — вырожденный запрос, ответ false (того же требует read_raw).
    virtual bool write_raw(std::uintptr_t addr, const void* src, std::uint32_t size) = 0;

    // Семантически == N вызовов write_raw, но одной пачкой. Исключений нет: провал одной
    // записи не роняет всю пачку, ответ у каждой записи свой (частичный ok по факту).
    // out короче reqs — ошибка вызывающего: пишется ровно min(reqs, out) записей, хвост
    // reqs не выполняется (списка «лишних» запросов интерфейс не даёт).
    virtual void write_batch(std::span<const WriteReq>, std::span<WriteResult> out) = 0;
};
}

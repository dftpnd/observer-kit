#pragma once

#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

#include "ovr/memory/memory_backend.hpp"
#include "ovr/memory/memory_writer.hpp"

#include "dump_loader.hpp"

namespace ovr::memory {

// «Память хоста» из фикстур: адресная карта + байты из файла дампа. Ни про смещения, ни про
// фичи не знает: вход — адрес, размер и карта (одна ответственность).
class FakeBackend final : public IMemoryBackend, public IMemoryWriter {
public:
    // Читает карту и байты один раз; false, если карты нет или она битая (E3).
    bool load(std::string_view map_path);

    bool attach(std::uint32_t pid) override;
    std::uintptr_t module_base(std::string_view module) const override;
    bool read_raw(std::uintptr_t addr, void* dst, std::uint32_t size) override;
    void read_batch(std::span<const ReadReq> reqs) override;
    bool write_raw(std::uintptr_t addr, const void* src, std::uint32_t size) override;
    void write_batch(std::span<const WriteReq>, std::span<WriteResult> out) override;

    // Подмены для тестов достоверности: чтения/записи внутри диапазона проваливаются.
    void set_fail_range(std::uintptr_t addr, std::uint32_t size);
    void set_write_fail_range(std::uintptr_t addr, std::uint32_t size);

    // Наблюдаемые счётчики: по ним тесты сверяют «одна пачка за кадр» и идемпотентность записи.
    std::uint32_t batch_calls() const;
    std::uint32_t write_calls() const;
    std::uint64_t write_bytes() const;
    std::uint32_t load_calls() const;

private:
    struct FailRange {
        std::uintptr_t begin = 0;
        std::uint32_t size = 0;
    };

    MapEntry* locate(std::uintptr_t addr, std::uint32_t size);
    bool blocked(const std::vector<FailRange>& ranges, std::uintptr_t addr, std::uint32_t size) const;

    std::vector<MapEntry> entries_;
    std::vector<FailRange> fail_ranges_;
    std::vector<FailRange> write_fail_ranges_;
    bool loaded_ = false;
    std::uint32_t batch_calls_ = 0;
    std::uint32_t write_calls_ = 0;
    std::uint32_t load_calls_ = 0;
    std::uint64_t write_bytes_ = 0;
};

}

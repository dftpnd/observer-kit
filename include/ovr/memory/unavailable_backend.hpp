#pragma once
#include "ovr/memory/memory_backend.hpp"
#include "ovr/memory/memory_writer.hpp"
namespace ovr::memory {
class UnavailableBackend final : public IMemoryBackend, public IMemoryWriter {
public:
    bool attach(std::uint32_t) override { return false; }
    std::uintptr_t module_base(std::string_view) const override { return 0; }
    bool read_raw(std::uintptr_t, void*, std::uint32_t) override { return false; }
    void read_batch(std::span<const ReadReq> reqs) override {
        for (const auto& req : reqs) if (req.result) *req.result = {};
    }
    bool write_raw(std::uintptr_t, const void*, std::uint32_t) override { return false; }
    void write_batch(std::span<const WriteReq>, std::span<WriteResult> out) override {
        for (auto& result : out) result = {};
    }
};
using RpmBackend = UnavailableBackend;
using ProcessVmReadvBackend = UnavailableBackend;
}

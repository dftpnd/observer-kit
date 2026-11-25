#pragma once
#include "ovr/state/parse_frame.hpp"
#include "ovr/memory/memory_backend.hpp"
namespace ovr::state {
class FrameSource {
public:
    FrameSource(memory::IMemoryBackend&, const offsets::OffsetRegistry&, FrameArena&) {}
    bool read_frame() { ++raw_.frame_seq; return false; }
    const memory::RawFrame& raw() const { return raw_; }
    std::uint32_t requests() const { return 0; }
private:
    memory::RawFrame raw_{};
};
}

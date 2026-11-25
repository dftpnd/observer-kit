#pragma once
#include "ovr/memory/raw_frame.hpp"
#include "ovr/state/frame_arena.hpp"
namespace ovr::offsets { class OffsetRegistry; }
namespace ovr::state {
inline HostState parse_frame(const memory::RawFrame&, const offsets::OffsetRegistry&, FrameArena& arena) {
    arena.begin_frame();
    return {};
}
}

#pragma once
#include "ovr/config/config.hpp"
#include "ovr/features/feature_registry.hpp"
#include "ovr/render/frame_draw.hpp"
#include "ovr/state/frame_arena.hpp"
#include "ovr/overlay/overlay.hpp"
#include "ovr/offsets/offset_registry.hpp"
#include "ovr/state/frame_source.hpp"
#include "ovr/memory/unavailable_backend.hpp"
#include <cstddef>
namespace ovr::offline {
class Session {
public:
    Session();
    std::size_t frame(const config::Config&);
private:
    features::FeatureRegistry features_;
    render::FrameDraw draw_;
    state::FrameArena arena_;
    offsets::OffsetRegistry offsets_;
    memory::UnavailableBackend backend_;
    state::FrameSource source_{backend_, offsets_, arena_};
    overlay::HeadlessOverlay overlay_;
};
}

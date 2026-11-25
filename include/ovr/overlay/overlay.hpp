#pragma once
#include "ovr/core/types.hpp"
#include <string_view>
namespace ovr::render { class FrameDraw; }
namespace ovr::overlay {
struct OverlayDesc { std::uint32_t width, height; bool topmost, transparent; std::string_view title; };
class IOverlay {
public:
    virtual ~IOverlay() = default;
    virtual bool init(const OverlayDesc&) = 0;
    virtual void resize(std::uint32_t, std::uint32_t) = 0;
    virtual void present(const render::FrameDraw&) = 0;
    virtual bool next_event(core::Vec2&, bool&) = 0;
};
class HeadlessOverlay final : public IOverlay {
public:
    bool init(const OverlayDesc&) override { return true; }
    void resize(std::uint32_t, std::uint32_t) override {}
    void present(const render::FrameDraw&) override {}
    bool next_event(core::Vec2&, bool&) override { return false; }
};
inline IOverlay* create_directx11() { return nullptr; }
inline IOverlay* create_vulkan() { return nullptr; }
}

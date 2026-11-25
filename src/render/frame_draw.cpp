#include "ovr/render/frame_draw.hpp"

#include <utility>

namespace ovr::render {
void FrameDraw::append(DrawItem item) {
    if (buffer_.capacity() == 0) {
        reserve(kDrawReserve);
    }
    buffer_.push_back(std::move(item));
}

void FrameDraw::line(core::Vec2 a, core::Vec2 b, std::uint32_t rgba, float thickness) {
    append(Line{a, b, rgba, thickness});
}

void FrameDraw::rect(core::Vec2 mn, core::Vec2 mx, std::uint32_t rgba, float thickness, bool fill) {
    append(Rect{mn, mx, rgba, thickness, fill});
}

void FrameDraw::circle(core::Vec2 c, float r, std::uint32_t rgba, float thickness, bool fill) {
    append(Circle{c, r, rgba, thickness, fill});
}

void FrameDraw::poly(std::span<const core::Vec2> pts, std::uint32_t rgba, float thickness, bool fill) {
    append(Poly{pts, rgba, thickness, fill});
}

void FrameDraw::text(core::Vec2 pos, std::string_view utf8, std::uint32_t rgba, float size) {
    append(Text{pos, rgba, size, utf8});
}

void FrameDraw::clear() { buffer_.clear(); }
std::span<const DrawItem> FrameDraw::items() const { return buffer_; }
void FrameDraw::reserve(std::size_t n) { buffer_.reserve(n); }
}

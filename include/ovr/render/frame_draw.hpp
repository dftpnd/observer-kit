#pragma once

#include "ovr/core/types.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <variant>
#include <vector>

namespace ovr::render {
// Минимальный холодный кадр E38: три команды. Бюджет рабочего кадра задаст замер T015.
inline constexpr std::uint32_t kDrawReserve = 3;

struct Line { core::Vec2 a, b; std::uint32_t rgba; float thickness; };
struct Rect { core::Vec2 min, max; std::uint32_t rgba; float thickness; bool fill; };
struct Circle { core::Vec2 center; float radius; std::uint32_t rgba; float thickness; bool fill; };
// Точки и байты текста принадлежат вызывающему и должны жить до конца кадра.
struct Poly { std::span<const core::Vec2> pts; std::uint32_t rgba; float thickness; bool fill; };
struct Text { core::Vec2 pos; std::uint32_t rgba; float size; std::string_view utf8; };
using DrawItem = std::variant<Line, Rect, Circle, Poly, Text>;

class FrameDraw {
public:
    void line(core::Vec2 a, core::Vec2 b, std::uint32_t rgba, float thickness = 1.f);
    void rect(core::Vec2 mn, core::Vec2 mx, std::uint32_t rgba, float thickness = 1.f, bool fill = false);
    void circle(core::Vec2 c, float r, std::uint32_t rgba, float thickness = 1.f, bool fill = false);
    void poly(std::span<const core::Vec2> pts, std::uint32_t rgba, float thickness = 1.f, bool fill = false);
    void text(core::Vec2 pos, std::string_view utf8, std::uint32_t rgba, float size = 13.f);
    void clear();
    std::span<const DrawItem> items() const;
    void reserve(std::size_t n);

private:
    void append(DrawItem item);
    std::vector<DrawItem> buffer_;
};
}

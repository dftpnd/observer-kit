#include "ovr/render/frame_draw.hpp"

#include <array>
#include <cstdlib>
#include <new>

namespace {
std::size_t allocations = 0;
}

void* operator new(std::size_t size) {
    if (void* ptr = std::malloc(size ? size : 1)) {
        ++allocations;
        return ptr;
    }
    throw std::bad_alloc();
}
void operator delete(void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }

int main() {
    using namespace ovr::render;
    FrameDraw draw;
    auto before = allocations;
    draw.reserve(0);
    if (allocations != before) return 1;
    draw.line({}, {}, 0);
    draw.circle({}, 0, 0);
    draw.rect({}, {}, 0);
    if (allocations != before + 1) return 1;
    draw.clear();
    draw.reserve(5);

    const std::array<ovr::core::Vec2, 2> points{{{1, 2}, {3, 4}}};
    constexpr std::string_view label = "frame";
    before = allocations;
    for (int i = 0; i < 100; ++i) {
        draw.reserve(0);
        draw.line(points[0], points[1], 1);
        draw.rect(points[0], points[1], 2, 2, true);
        draw.circle(points[1], 3, 3);
        draw.poly(points, 4);
        draw.text(points[1], label, 5);
        const auto items = draw.items();
        if (items.size() != 5 || std::get<Line>(items[0]).a.y != 2 ||
            !std::get<Rect>(items[1]).fill || std::get<Circle>(items[2]).radius != 3 ||
            std::get<Poly>(items[3]).pts.data() != points.data() ||
            std::get<Text>(items[4]).utf8.data() != label.data()) return 1;
        draw.clear();
        if (!draw.items().empty() || allocations != before) return 1;
    }
    return 0;
}

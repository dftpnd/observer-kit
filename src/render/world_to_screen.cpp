#include "ovr/render/world_to_screen.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <iterator>

namespace ovr::render {
namespace {
bool usable(const View& v) {
    return v.valid && v.width && v.height && std::isfinite(v.aspect) && v.aspect > 0 &&
           std::isfinite(v.fov_deg) && v.fov_deg > 0 && v.fov_deg < 180 &&
           std::isfinite(v.znear) && std::isfinite(v.zfar) && v.znear < v.zfar &&
           std::all_of(std::begin(v.matrix.m), std::end(v.matrix.m),
                       [](float x) { return std::isfinite(x); });
}
}

bool world_to_screen(const View& v, core::Vec3 p, core::Vec2& out) {
    if (!usable(v)) return false;
    const auto clip = [&](unsigned row) {
        const auto* a = v.matrix.m + 4 * row;
        return double(a[0]) * p.x + double(a[1]) * p.y + double(a[2]) * p.z + a[3];
    };
    const double w = clip(3);
    if (!(w > 0)) return false;
    const core::Vec2 pixel{static_cast<float>((clip(0) / w + 1) * v.width / 2),
                           static_cast<float>((1 - clip(1) / w) * v.height / 2)};
    if (!(pixel.x >= 0 && pixel.x < double(v.width) && pixel.y >= 0 && pixel.y < double(v.height)))
        return false;
    out = pixel;
    return true;
}

bool screen_to_world_ray(const View& v, core::Vec2 p, core::Vec3& out) {
    if (!usable(v) || !(p.x >= 0 && p.x < double(v.width) && p.y >= 0 && p.y < double(v.height)))
        return false;
    const double x = 2.0 * p.x / v.width - 1;
    const double y = 1 - 2.0 * p.y / v.height;
    // A perspective ray intersects the two planes clip.x = x*w and clip.y = y*w.
    std::array<double, 3> a{}, b{};
    const auto* m = v.matrix.m;
    for (unsigned j = 0; j < 3; ++j) {
        a[j] = m[j] - x * m[12+j];
        b[j] = m[4+j] - y * m[12+j];
    }
    const double dx = a[1]*b[2] - a[2]*b[1];
    const double dy = a[2]*b[0] - a[0]*b[2];
    const double dz = a[0]*b[1] - a[1]*b[0];
    const double forward = dx*m[12] + dy*m[13] + dz*m[14];
    if (forward == 0) return false;  // no unique forward perspective ray
    const double length = std::copysign(std::hypot(dx, dy, dz), forward);
    out = {static_cast<float>(dx/length), static_cast<float>(dy/length), static_cast<float>(dz/length)};
    return true;
}
}

#include "ovr/core/key.hpp"
#include "ovr/core/time.hpp"
#include "ovr/core/types.hpp"
#include "ovr/core/vec.hpp"

#include <cmath>
#include <cstdint>
#include <optional>
#include <string_view>

using namespace ovr::core;

namespace {

int failures = 0;

void check(bool condition) {
    if (!condition) {
        ++failures;
    }
}

bool close(float a, float b, float eps = 1e-5f) {
    return std::fabs(a - b) <= eps;
}

bool close(const Vec3& a, const Vec3& b, float eps = 1e-5f) {
    return close(a.x, b.x, eps) && close(a.y, b.y, eps) && close(a.z, b.z, eps);
}

bool close(const Vec2& a, const Vec2& b, float eps = 1e-5f) {
    return close(a.x, b.x, eps) && close(a.y, b.y, eps);
}

bool matrix_close(const Mat4x4& a, const Mat4x4& b, float eps = 1e-5f) {
    for (int i = 0; i < 16; ++i) {
        if (!close(a.m[i], b.m[i], eps)) {
            return false;
        }
    }
    return true;
}

bool finite(const Vec3& v) {
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}

bool finite(const Vec2& v) {
    return std::isfinite(v.x) && std::isfinite(v.y);
}

Mat4x4 identity() {
    Mat4x4 m;
    m.m[0] = 1.0f;
    m.m[5] = 1.0f;
    m.m[10] = 1.0f;
    m.m[15] = 1.0f;
    return m;
}

Mat4x4 sample_matrix(int seed) {
    Mat4x4 m;
    for (int i = 0; i < 16; ++i) {
        m.m[i] = static_cast<float>((i * 7 + seed * 13) % 9 - 4) * 0.1f;
    }
    return m;
}

void check_vectors() {
    check(close(dot(Vec2{1.0f, 2.0f}, Vec2{3.0f, 4.0f}), 11.0f));
    check(close(length(Vec3{3.0f, 4.0f, 0.0f}), 5.0f));
    check(close(length_sq(Vec2{2.0f, 3.0f}), 13.0f));
    check(close(Vec2{1.0f, 1.0f} + Vec2{2.0f, 4.0f}, Vec2{3.0f, 5.0f}));
    check(close(Vec2{1.0f, 1.0f} - Vec2{2.0f, 4.0f}, Vec2{-1.0f, -3.0f}));
    check(close(Vec2{1.0f, 1.0f} * 3.0f, Vec2{3.0f, 3.0f}));
    check(close(normalize(Vec3{3.0f, 0.0f, 0.0f}), Vec3{1.0f, 0.0f, 0.0f}));
    check(close(normalize(Vec2{0.0f, 5.0f}), Vec2{0.0f, 1.0f}));

    check(close(normalize(Vec2{0x1p-100f, 0.0f}), Vec2{1.0f, 0.0f}));
    check(close(normalize(Vec3{0.0f, 0.0f, 0x1p100f}), Vec3{0.0f, 0.0f, 1.0f}));
    const float largest = 0x1.fffffep127f;
    check(close(normalize(Vec2{largest, largest}), Vec2{std::sqrt(0.5f), std::sqrt(0.5f)}));

    const Vec3 zero_norm = normalize(Vec3{});
    check(close(zero_norm, Vec3{0.0f, 0.0f, 0.0f}));
    check(finite(zero_norm));
    const Vec2 zero_norm2 = normalize(Vec2{});
    check(close(zero_norm2, Vec2{0.0f, 0.0f}));
    check(finite(zero_norm2));
    const float nan = std::nanf("");
    const float inf = INFINITY;
    check(finite(normalize(Vec3{nan, inf, 1.0f})));
    check(finite(normalize(Vec2{inf, nan})));
}

void check_lerp() {
    const float nan = std::nanf("");
    check(close(lerp(Vec2{1.0f, 2.0f}, Vec2{3.0f, 6.0f}, 0.5f), Vec2{2.0f, 4.0f}));
    check(close(lerp(Vec2{1.0f, 2.0f}, Vec2{3.0f, 6.0f}, nan), Vec2{1.0f, 2.0f}));
    check(close(lerp(Vec2{nan, 1.0f}, Vec2{3.0f, 6.0f}, nan), Vec2{3.0f, 1.0f}));
    const float largest = 0x1.fffffep127f;
    check(close(lerp(Vec2{-largest, largest}, Vec2{largest, -largest}, 0.5f), Vec2{}));
    const Vec2 nan_a = lerp(Vec2{nan, 1.0f}, Vec2{3.0f, 6.0f}, 0.5f);
    check(finite(nan_a));
    const Vec2 nan_b = lerp(Vec2{1.0f, 2.0f}, Vec2{3.0f, nan}, 0.5f);
    check(finite(nan_b));
    const Vec2 both_nan = lerp(Vec2{nan, nan}, Vec2{nan, nan}, 0.5f);
    check(close(both_nan, Vec2{0.0f, 0.0f}));
    check(finite(lerp(Vec2{1.0f, 2.0f}, Vec2{3.0f, 6.0f}, INFINITY)));
}

void check_matrix() {
    static_assert(sizeof(Mat4x4) == 64);
    Mat4x4 zero;
    for (int i = 0; i < 16; ++i) {
        check(zero.m[i] == 0.0f);
    }

    const Mat4x4 id = identity();
    const Vec3 p{1.0f, 2.0f, 3.0f};
    check(close(transform_point(id, p), p));

    const Vec3 behind = transform_point(zero, p);
    check(close(behind, Vec3{0.0f, 0.0f, 0.0f}));
    check(finite(behind));

    Mat4x4 flipped = id;
    flipped.m[15] = -1.0f;
    check(close(transform_point(flipped, p), Vec3{0.0f, 0.0f, 0.0f}));

    Mat4x4 shifted = id;
    shifted.m[3] = 100.0f;
    check(close(transform_dir(shifted, p), p));
    check(close(transform_point(shifted, Vec3{}), Vec3{100.0f, 0.0f, 0.0f}));

    const Mat4x4 m = sample_matrix(1);
    check(matrix_close(m * id, m));
    check(matrix_close(id * m, m));

    const Mat4x4 a = sample_matrix(2);
    const Mat4x4 b = sample_matrix(3);
    const Mat4x4 c = sample_matrix(4);
    check(matrix_close((a * b) * c, a * (b * c)));
}

void check_time() {
    check(ms_to_frames(0.0, 60.0) == 0);
    check(ms_to_frames(100.0, 0.0) == 0);
    check(ms_to_frames(100.0, -1.0) == 0);
    check(ms_to_frames(-20.0, 60.0) == 0);
    check(ms_to_frames(16.0, 60.0) == 1);
    check(ms_to_frames(5.0, 60.0) == 1);
    check(ms_to_frames(16.7, 60.0) == 2);
    check(ms_to_frames(1000.0, 60.0) == 60);

    check(ms_to_frames(0x1p-1022, 0x1p-1022) == 1);
    check(ms_to_frames(1.0e200, 1.0e200) == UINT32_MAX);

    const std::uint64_t first = now_ns();
    const std::uint64_t second = now_ns();
    check(second >= first);
}

void check_keys() {
    const std::optional<Key> insert = key_from_name("INSERT");
    check(insert.has_value());
    check(static_cast<std::uint16_t>(insert.value_or(Key::unknown)) == 0x2d);
    check(insert.value_or(Key::unknown) == Key::insert);
    check(!key_from_name("insert").has_value());
    check(!key_from_name("nope").has_value());
    check(!key_from_name("").has_value());

    const std::optional<Key> mouse = key_from_name("MOUSE5");
    check(mouse.has_value());
    check(static_cast<std::uint16_t>(mouse.value_or(Key::unknown)) == 5);

    for (const std::string_view name : {"INSERT", "MOUSE5"}) {
        check(key_name(key_from_name(name).value_or(Key::unknown)) == name);
    }
    check(key_name(Key::unknown).empty());
}

}  // namespace

int main() {
    check_vectors();
    check_lerp();
    check_matrix();
    check_time();
    check_keys();
    return failures == 0 ? 0 : 1;
}

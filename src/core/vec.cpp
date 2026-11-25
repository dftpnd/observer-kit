#include "ovr/core/vec.hpp"

#include <cmath>

namespace ovr::core {
namespace {

float blend(float a, float b, float t) {
    if (!std::isfinite(a) && !std::isfinite(b)) {
        return 0.0f;
    }
    if (!std::isfinite(a)) {
        return b;
    }
    if (!std::isfinite(b)) {
        return a;
    }
    const float result = a + (b - a) * t;
    return std::isfinite(result) ? result : a;
}

}  // namespace

Vec2 operator+(Vec2 a, Vec2 b) { return {a.x + b.x, a.y + b.y}; }
Vec2 operator-(Vec2 a, Vec2 b) { return {a.x - b.x, a.y - b.y}; }
Vec2 operator*(Vec2 a, float k) { return {a.x * k, a.y * k}; }

float dot(Vec2 a, Vec2 b) { return a.x * b.x + a.y * b.y; }
float length_sq(Vec2 a) { return dot(a, a); }
float length(Vec2 a) { return std::sqrt(dot(a, a)); }

Vec2 normalize(Vec2 a) {
    const float len = length(a);
    if (!std::isfinite(len) || len == 0.0f) {
        return {};
    }
    return {a.x / len, a.y / len};
}

Vec2 lerp(Vec2 a, Vec2 b, float t) {
    if (!std::isfinite(t)) {
        return a;
    }
    return {blend(a.x, b.x, t), blend(a.y, b.y, t)};
}

Vec3 operator+(Vec3 a, Vec3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
Vec3 operator-(Vec3 a, Vec3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
Vec3 operator*(Vec3 a, float k) { return {a.x * k, a.y * k, a.z * k}; }

float dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
float length_sq(Vec3 a) { return dot(a, a); }
float length(Vec3 a) { return std::sqrt(dot(a, a)); }

Vec3 normalize(Vec3 a) {
    const float len = length(a);
    if (!std::isfinite(len) || len == 0.0f) {
        return {};
    }
    return {a.x / len, a.y / len, a.z / len};
}

Mat4x4 operator*(const Mat4x4& a, const Mat4x4& b) {
    Mat4x4 out;
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            float sum = 0.0f;
            for (int step = 0; step < 4; ++step) {
                sum += a.m[row * 4 + step] * b.m[step * 4 + col];
            }
            out.m[row * 4 + col] = sum;
        }
    }
    return out;
}

Vec3 transform_point(const Mat4x4& m, Vec3 p) {
    const float w = m.m[12] * p.x + m.m[13] * p.y + m.m[14] * p.z + m.m[15];
    if (!(w > 0.0f)) {
        return {};
    }
    const float x = m.m[0] * p.x + m.m[1] * p.y + m.m[2] * p.z + m.m[3];
    const float y = m.m[4] * p.x + m.m[5] * p.y + m.m[6] * p.z + m.m[7];
    const float z = m.m[8] * p.x + m.m[9] * p.y + m.m[10] * p.z + m.m[11];
    return {x / w, y / w, z / w};
}

Vec3 transform_dir(const Mat4x4& m, Vec3 d) {
    return {m.m[0] * d.x + m.m[1] * d.y + m.m[2] * d.z,
            m.m[4] * d.x + m.m[5] * d.y + m.m[6] * d.z,
            m.m[8] * d.x + m.m[9] * d.y + m.m[10] * d.z};
}
}

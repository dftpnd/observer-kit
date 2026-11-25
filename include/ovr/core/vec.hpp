#pragma once

#include "ovr/core/types.hpp"

namespace ovr::core {
Vec2 operator+(Vec2, Vec2);
Vec2 operator-(Vec2, Vec2);
Vec2 operator*(Vec2, float);
float dot(Vec2, Vec2);
float length_sq(Vec2);
float length(Vec2);
Vec2 normalize(Vec2);   // {0,0,0} => {0,0,0}: ни NaN, ни деления на ноль (E43)
Vec2 lerp(Vec2, Vec2, float t);   // t не конечное => a, NaN наружу не выходит (E43)

Vec3 operator+(Vec3, Vec3);
Vec3 operator-(Vec3, Vec3);
Vec3 operator*(Vec3, float);
float dot(Vec3, Vec3);
float length_sq(Vec3);
float length(Vec3);
Vec3 normalize(Vec3);   // тот же контракт, что у Vec2 (E43)

Mat4x4 operator*(const Mat4x4&, const Mat4x4&);
Vec3 transform_point(const Mat4x4&, Vec3);   // с делением на w; w <= 0 => {0,0,0} (E43)
Vec3 transform_dir(const Mat4x4&, Vec3);     // без деления на w: только направление
}

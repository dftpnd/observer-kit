#pragma once

#include <cstdint>

namespace ovr::core {
struct Vec2 { float x{}, y{}; };
struct Vec3 { float x{}, y{}, z{}; };
struct Mat4x4 { float m[16]{}; };   // row-major, как кладёт Source2
using ModuleId = std::uint8_t;      // индекс в таблице модулей реестра
constexpr std::uint64_t kInvalidAddr = 0;
}

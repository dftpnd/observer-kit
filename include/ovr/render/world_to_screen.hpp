#pragma once
#include "ovr/core/types.hpp"

namespace ovr::render {
struct View {
    core::Mat4x4 matrix;
    core::Vec3 cam_origin;
    float fov_deg{}, aspect{}, znear{}, zfar{};
    bool valid{};
    std::uint32_t width{}, height{};
};
bool world_to_screen(const View&, core::Vec3 world, core::Vec2& out_screen);
bool screen_to_world_ray(const View&, core::Vec2 screen, core::Vec3& out_dir);
}

#pragma once
#include "ovr/core/types.hpp"
#include <span>
#include <string>

namespace ovr::state {
struct ActorSnapshot {
    core::Vec3 pos{};
    int team{-1}, health{-1}, max_health{-1}, mana{-1}, max_mana{-1}, level{-1};
    std::uint32_t flags{};
    bool alive{}, visible_to_us{};
};
struct Agent {
    std::uint32_t raw_index{};
    core::Vec3 pos{};
    int team{-1}, health{-1}, max_health{-1}, gold{-1}, last_hits{-1}, xp{-1};
    std::uint32_t flags{};
    bool alive{}, is_actor{}, is_minion{}, is_guard{}, is_building{}, is_beacon{};
    std::uint16_t entity_type{};
};
struct HostState {
    int tick_client{-1}, tick_server{-1};
    double current_time{};
    int map_width{-1}, map_height{-1};
    core::Vec3 camera_origin{};
    core::Mat4x4 view_matrix{};
    float fov_deg{}, aspect{}, znear{}, zfar{};
    std::span<ActorSnapshot> actors{};
    std::span<Agent> agents{};
    std::uint64_t validity_bits{};
    bool is_background_map{};
    int sign_on_state{-1};
};
std::string to_string(const ActorSnapshot&);
std::string to_string(const Agent&);
}

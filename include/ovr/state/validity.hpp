#pragma once
#include <cstdint>

namespace ovr::state {
enum class FieldBit : std::uint8_t {
  // 0..15 — кадр, сеть, камера
  frame_tick_client = 0, frame_tick_server, frame_time, frame_map_size,
  frame_background_map, frame_sign_on, camera_origin, view_matrix,
  camera_fov_aspect, camera_clip,
  frame_reserved0, frame_reserved1, frame_reserved2, frame_reserved3,
  frame_reserved4, frame_reserved5,
  // 16..39 — поля актора
  actor_pos = 16, actor_team, actor_health, actor_max_health, actor_mana, actor_max_mana,
  actor_level, actor_flags, actor_alive, actor_visible,
  actor_reserved0, actor_reserved1, actor_reserved2, actor_reserved3, actor_reserved4,
  actor_reserved5, actor_reserved6, actor_reserved7, actor_reserved8, actor_reserved9,
  actor_reserved10, actor_reserved11, actor_reserved12, actor_reserved13,
  // 40..63 — поля агента
  agent_index = 40, agent_pos, agent_team, agent_health, agent_max_health, agent_gold,
  agent_last_hits, agent_xp, agent_flags, agent_alive, agent_type,
  agent_reserved0, agent_reserved1, agent_reserved2, agent_reserved3, agent_reserved4,
  agent_reserved5, agent_reserved6, agent_reserved7, agent_reserved8, agent_reserved9,
  agent_reserved10, agent_reserved11, agent_reserved12
};
constexpr bool field_ok(std::uint64_t bits, FieldBit field) {
    const auto index = static_cast<unsigned>(field);
    return index < 64 && ((bits >> index) & 1u) != 0;
}
}

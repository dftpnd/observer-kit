#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

namespace ovr::core {
enum class Key : std::uint16_t {
    unknown = 0 /* VK-коды Windows, коды Linux; MOUSE5 == 5 */,
    insert = 29,
    mouse5 = 5,
};

std::optional<Key> key_from_name(std::string_view name);
std::string_view key_name(Key key);
}

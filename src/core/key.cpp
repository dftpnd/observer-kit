#include "ovr/core/key.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string_view>

namespace ovr::core {
namespace {

struct Entry {
    std::string_view name;
    Key key;
};

constexpr std::array<Entry, 2> kKeys{{{"INSERT", Key::insert}, {"MOUSE5", Key::mouse5}}};

}  // namespace

std::optional<Key> key_from_name(std::string_view name) {
    for (const auto& entry : kKeys) {
        if (entry.name == name) {
            return entry.key;
        }
    }
    return {};
}

std::string_view key_name(Key key) {
    for (const auto& entry : kKeys) {
        if (entry.key == key) {
            return entry.name;
        }
    }
    return {};
}
}

#pragma once
#include "ovr/offsets/offset_registry.hpp"
#include <span>
#include <type_traits>
namespace ovr::memory {
struct ChainStep { offsets::Symbol sym; };
template<class T> struct Typed { T value{}; bool ok{}; };
class TypedReader {
public:
    template<class T>
    Typed<T> read(const offsets::OffsetRegistry&, std::string_view,
                  std::span<const ChainStep> = {}) const {
        static_assert(std::is_trivially_copyable_v<T> && sizeof(T) <= 64);
        return {};
    }
};
}

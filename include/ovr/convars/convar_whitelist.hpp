#pragma once
#include "ovr/memory/memory_writer.hpp"
namespace ovr::memory { class IMemoryBackend; }
namespace ovr::config { struct Config; }
namespace ovr::offsets { class OffsetRegistry; }
namespace ovr::convars {
inline bool apply_from_config(const config::Config&, memory::IMemoryBackend&,
                              memory::IMemoryWriter&, const offsets::OffsetRegistry&) { return false; }
inline std::uint32_t restore_all(memory::IMemoryBackend&, memory::IMemoryWriter&) { return 0; }
}

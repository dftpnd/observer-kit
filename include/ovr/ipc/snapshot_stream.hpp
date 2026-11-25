#pragma once
#include <memory>
#include <string_view>
namespace ovr::state { struct HostState; }
namespace ovr::ipc {
class SnapshotStream {
public:
    virtual ~SnapshotStream() = default;
    virtual bool publish(const state::HostState&) = 0;
    virtual bool try_consume(state::HostState&) = 0;
};
class UnavailableStream final : public SnapshotStream {
public:
    bool publish(const state::HostState&) override { return false; }
    bool try_consume(state::HostState&) override { return false; }
};
inline std::unique_ptr<SnapshotStream> create_shm_ring(std::string_view) {
    return std::make_unique<UnavailableStream>();
}
}

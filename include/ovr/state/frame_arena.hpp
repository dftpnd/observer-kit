#pragma once
#include "ovr/state/host_state.hpp"
#include <array>
#include <limits>
#include <stdexcept>
#include <vector>

namespace ovr::state {
inline constexpr std::uint32_t kMaxActors = 10;
inline constexpr std::uint32_t kMaxAgents = 64;

class FrameArena {
public:
    explicit FrameArena(std::uint32_t max_clients = 0) {
        if (max_clients > std::numeric_limits<std::uint32_t>::max() - kMaxAgents)
            throw std::length_error("max_clients");
        agents_.resize(kMaxAgents + max_clients);
    }
    FrameArena(const FrameArena&) = delete;
    FrameArena& operator=(const FrameArena&) = delete;
    void begin_frame() noexcept {
        used_actors_ = used_agents_ = 0;
        overflowed_ = false;
    }
    std::span<ActorSnapshot> actors() noexcept { return actors_; }
    std::span<Agent> agents() noexcept { return agents_; }
    std::uint32_t used_actors() const noexcept { return used_actors_; }
    std::uint32_t used_agents() const noexcept { return used_agents_; }
    bool overflowed() const noexcept { return overflowed_; }
    bool append(const ActorSnapshot& actor) noexcept {
        if (used_actors_ == actors_.size()) {
            overflowed_ = true;
            return false;
        }
        actors_[used_actors_++] = actor;
        return true;
    }
    bool append(const Agent& agent) noexcept {
        if (used_agents_ == agents_.size()) {
            overflowed_ = true;
            return false;
        }
        agents_[used_agents_++] = agent;
        return true;
    }
private:
    std::array<ActorSnapshot, kMaxActors> actors_{};
    std::vector<Agent> agents_;
    std::uint32_t used_actors_{}, used_agents_{};
    bool overflowed_{};
};
}

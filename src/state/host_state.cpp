#include "ovr/state/host_state.hpp"

namespace ovr::state {
std::string to_string(const Agent& agent) {
    return "agent " + std::to_string(agent.raw_index) + " team=" + std::to_string(agent.team);
}
std::string to_string(const ActorSnapshot& actor) {
    return "actor team=" + std::to_string(actor.team) + " health=" + std::to_string(actor.health);
}
}

#include "ovr/state/frame_arena.hpp"
#include "ovr/state/validity.hpp"
#include <cstdlib>
#include <new>
#include <type_traits>

using namespace ovr::state;

namespace {
unsigned allocations;
void check(bool success) { if (!success) std::abort(); }
}
void* operator new(std::size_t bytes) {
    ++allocations;
    if (void* p = std::malloc(bytes ? bytes : 1)) return p;
    throw std::bad_alloc{};
}
void operator delete(void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }

int main() {
    static_assert(std::is_standard_layout_v<Agent> && std::is_standard_layout_v<ActorSnapshot>);
    static_assert(std::is_trivially_copyable_v<HostState>);
    static_assert(!std::is_copy_constructible_v<FrameArena>);
    static_assert(field_ok(1ull << 63, FieldBit::agent_reserved12));
    static_assert(!field_ok(~0ull, static_cast<FieldBit>(64)));
    HostState state;
    check(state.validity_bits == 0 && state.actors.empty() && state.agents.empty());
    FrameArena arena(2);
    check(arena.agents().size() == kMaxAgents + 2);
    const unsigned before = allocations;
    for (unsigned i = 0; i < 100; ++i) {
        arena.begin_frame();
        check(!arena.overflowed() && arena.used_actors() == 0 && arena.used_agents() == 0);
        for (unsigned j = 0; j < kMaxActors; ++j) check(arena.append(ActorSnapshot{}));
        check(!arena.append(ActorSnapshot{}) && arena.overflowed());
        for (unsigned j = 0; j < arena.agents().size(); ++j) check(arena.append(Agent{}));
        check(!arena.append(Agent{}));
    }
    check(allocations == before);
    state.actors = arena.actors().first(arena.used_actors());
    state.agents = arena.agents().first(arena.used_agents());
    state.current_time = 2;
    const auto copy = state;
    state.current_time = 1;
    check(copy.current_time == 2 && state.current_time == 1);
    check(copy.agents.data() == state.agents.data() && copy.actors.data() == state.actors.data());
    check(copy.agents.front().gold == -1 && copy.actors.front().health == -1);
}

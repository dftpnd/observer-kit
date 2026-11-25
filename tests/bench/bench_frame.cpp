#include "ovr/offline/session.hpp"
#include "ovr/state/frame_arena.hpp"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <string_view>

int main(int argc, char** argv) {
    const bool smoke = argc == 2 && std::string_view(argv[1]) == "--smoke";
    if (argc > 1 && !smoke) return 2;

    constexpr unsigned agents = 400;  // E35
    const unsigned frames = smoke ? 1 : 1000;
    ovr::offline::Session session;
    ovr::config::Config cfg;
    ovr::state::FrameArena arena(agents - ovr::state::kMaxAgents);
    ovr::render::FrameDraw draw;
    draw.reserve(agents);
    using Clock = std::chrono::steady_clock;
    using Ms = std::chrono::duration<double, std::milli>;
    Ms session_time{}, state_time{}, draw_time{};
    std::size_t commands = 0;
    session.frame(cfg);

    for (unsigned frame = 0; frame < frames; ++frame) {
        const auto start = Clock::now();
        if (session.frame(cfg) != 0) return 1;
        const auto session_end = Clock::now();
        arena.begin_frame();
        for (unsigned i = 0; i < agents; ++i) {
            ovr::state::Agent agent{};
            agent.raw_index = i;
            agent.pos.x = static_cast<float>(i);
            if (!arena.append(agent)) return 1;
        }
        const auto state_end = Clock::now();
        draw.clear();
        for (const auto& agent : arena.agents().first(arena.used_agents())) {
            draw.line({}, {agent.pos.x, agent.pos.y}, 0);
        }
        commands += draw.items().size();
        const auto end = Clock::now();
        session_time += session_end - start;
        state_time += state_end - session_end;
        draw_time += end - state_end;
    }
    const double mean = (session_time + state_time + draw_time).count() / frames;
    std::cout << std::fixed << std::setprecision(6)
              << "mode=synthetic frames=" << frames << " agents_per_frame=" << agents << '\n'
              << "read=not-measured parse=not-measured gpu=not-measured\n"
              << "offline_empty_session=" << session_time.count() / frames << " ms\n"
              << "synthetic_arena_fill=" << state_time.count() / frames << " ms\n"
              << "synthetic_cpu_draw_queue=" << draw_time.count() / frames << " ms\n"
              << "commands=" << commands << '\n'
              << "total=" << mean << " ms per synthetic frame\n";
    return commands == agents * frames ? 0 : 1;
}

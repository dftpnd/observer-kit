#include "ovr/offsets/offset_registry.hpp"
#include "ovr/state/parse_frame.hpp"
#include "ovr/state/validity.hpp"

#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

int main(int argc, char** argv) {
    if (argc != 2) return 2;
    std::ifstream input(argv[1], std::ios::binary);
    if (!input) {
        std::cerr << "not-established: " << argv[1] << '\n';
        return 1;
    }
    std::vector<std::uint8_t> bytes(std::istreambuf_iterator<char>(input), {});
    if (input.bad()) {
        std::cerr << "raw: " << argv[1] << '\n';
        return 1;
    }

    using namespace ovr::state;
    FrameArena arena;
    arena.append(ActorSnapshot{});
    arena.append(Agent{});
    const ovr::memory::RawFrame raw{bytes, 0, 0, false};
    const HostState state = parse_frame(raw, ovr::offsets::OffsetRegistry{}, arena);
    auto number_or_null = [&](FieldBit bit, int number) {
        return field_ok(state.validity_bits, bit) ? std::to_string(number) : "null";
    };
    std::cout << "{\"tick_client\":" << number_or_null(FieldBit::frame_tick_client, state.tick_client)
              << ",\"tick_server\":" << number_or_null(FieldBit::frame_tick_server, state.tick_server)
              << ",\"sign_on_state\":" << number_or_null(FieldBit::frame_sign_on, state.sign_on_state)
              << ",\"validity_bits\":" << state.validity_bits
              << ",\"actors\":" << state.actors.size()
              << ",\"agents\":" << state.agents.size() << "}\n";
    if (arena.used_actors() || arena.used_agents()) {
        std::cerr << "field=arena expected=empty actual=nonempty\n";
        return 1;
    }
}

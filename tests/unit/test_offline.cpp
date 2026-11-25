#include "ovr/convars/convar_whitelist.hpp"
#include "ovr/input/input_source.hpp"
#include "ovr/ipc/snapshot_stream.hpp"
#include "ovr/memory/unavailable_backend.hpp"
#include "ovr/memory/typed_reader.hpp"
#include "ovr/offline/session.hpp"
#include "ovr/state/frame_source.hpp"
#include <array>
int main() {
    using namespace ovr;
    memory::UnavailableBackend memory;
    offsets::OffsetRegistry offsets;
    config::Config cfg;
    unsigned char value = 42;
    memory::ReadResult result{&value, 1, true};
    const std::array reads{memory::ReadReq{0, &value, 1, &result}};
    memory.read_batch(reads);
    if (value != 42 || result.ok || result.data || result.size || memory.attach(1)) return 1;
    if (memory::TypedReader{}.read<int>(offsets, "missing").ok) return 1;
    if (convars::apply_from_config(cfg, memory, memory, offsets) || convars::restore_all(memory, memory)) return 1;
    state::FrameArena arena;
    state::FrameSource source(memory, offsets, arena);
    if (source.read_frame() || source.requests() || source.raw().complete) return 1;
    auto state = state::parse_frame(source.raw(), offsets, arena);
    if (state.validity_bits || !state.agents.empty()) return 1;
    auto stream = ipc::create_shm_ring("offline");
    state.tick_client = 31;
    if (stream->publish(state) || stream->try_consume(state) || state.tick_client != 31) return 1;
    auto input = input::create_input_source(nullptr, {});
    if (input->press(core::Key::insert) || input->release(core::Key::insert) || input->is_down(core::Key::insert)) return 1;
    return offline::Session{}.frame(cfg) == 0 ? 0 : 1;
}

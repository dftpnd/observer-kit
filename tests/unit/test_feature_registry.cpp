#include "ovr/offline/session.hpp"
#include <array>
#include <stdexcept>
namespace {
struct Trace { std::array<char, 3> names{}; unsigned size{}; };
class Probe final : public ovr::features::IFeature {
public:
    Probe(std::string_view id, Trace& trace) : id_(id), trace_(trace) {}
    std::string_view name() const override { return id_; }
    void run(const ovr::state::HostState&, const ovr::config::Config&, ovr::render::FrameDraw&) override {
        trace_.names.at(trace_.size++) = id_.front();
        if (id_ == "b") throw std::runtime_error("test");
    }
private:
    std::string_view id_;
    Trace& trace_;
};
}
int main() {
    ovr::features::FeatureRegistry registry;
    Trace trace;
    for (auto id : {"c", "b", "a", "b"}) registry.add(std::make_unique<Probe>(id, trace));
    if (registry.features().size() != 3) return 1;
    ovr::config::Config cfg;
    ovr::render::FrameDraw draw;
    cfg.markers_enabled = false;
    registry.run_all({}, cfg, draw);
    if (trace.size) return 1;
    cfg.markers_enabled = true;
    registry.run_all({}, cfg, draw);
    return trace.size == 3 && trace.names == std::array{'a', 'b', 'c'} && draw.items().empty() ? 0 : 1;
}

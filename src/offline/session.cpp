#include "ovr/offline/session.hpp"
#include "ovr/state/parse_frame.hpp"
namespace ovr::offline {
namespace {
class UnavailableFeature final : public features::IFeature {
public:
    explicit UnavailableFeature(std::string_view id) : id_(id) {}
    std::string_view name() const override { return id_; }
    void run(const state::HostState&, const config::Config&, render::FrameDraw&) override {}
private:
    std::string_view id_;
};
}
Session::Session() {
    for (auto id : {"vision", "last_hit", "timers", "skeleton", "snap_lines", "fow_overlay",
                    "range_display", "cooldowns", "illusions", "alerts", "combo"}) {
        features_.add(std::make_unique<UnavailableFeature>(id));
    }
}
std::size_t Session::frame(const config::Config& cfg) {
    draw_.clear();
    source_.read_frame();
    const auto empty = state::parse_frame(source_.raw(), offsets_, arena_);
    features_.run_all(empty, cfg, draw_);
    overlay_.present(draw_);
    return draw_.items().size();
}
}

#include "ovr/features/feature_registry.hpp"
#include "ovr/config/config.hpp"
#include <algorithm>
#include <iostream>
namespace ovr::features {
void FeatureRegistry::add(std::unique_ptr<IFeature> feature) {
    if (!feature) return;
    const auto name = feature->name();
    const auto it = std::lower_bound(order_.begin(), order_.end(), name,
        [](const IFeature* item, std::string_view key) { return item->name() < key; });
    if (it != order_.end() && (*it)->name() == name) {
        std::cerr << "duplicate feature: " << name << '\n';
        return;
    }
    owners_.reserve(owners_.size() + 1);
    order_.insert(it, feature.get());
    owners_.push_back(std::move(feature));
}
void FeatureRegistry::run_all(const state::HostState& state, const config::Config& cfg,
                              render::FrameDraw& draw) {
    if (!cfg.markers_enabled) return;
    for (auto* feature : order_) {
        try { feature->run(state, cfg, draw); }
        catch (...) { std::cerr << "feature failed: " << feature->name() << '\n'; }
    }
}
}

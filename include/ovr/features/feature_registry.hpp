#pragma once
#include "ovr/features/feature.hpp"
#include <memory>
#include <span>
#include <vector>
namespace ovr::features {
class FeatureRegistry {
public:
    void add(std::unique_ptr<IFeature>);
    void run_all(const state::HostState&, const config::Config&, render::FrameDraw&);
    std::span<IFeature* const> features() const { return order_; }
private:
    std::vector<std::unique_ptr<IFeature>> owners_;
    std::vector<IFeature*> order_;
};
}

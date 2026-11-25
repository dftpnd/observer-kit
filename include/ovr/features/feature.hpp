#pragma once
#include <string_view>
namespace ovr::state { struct HostState; }
namespace ovr::config { struct Config; }
namespace ovr::render { class FrameDraw; }
namespace ovr::features {
class IFeature {
public:
    virtual ~IFeature() = default;
    virtual std::string_view name() const = 0;
    virtual void run(const state::HostState&, const config::Config&, render::FrameDraw&) = 0;
};
}

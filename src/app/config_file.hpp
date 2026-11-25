#pragma once
#include "cli.hpp"
#include "ovr/config/config.hpp"
#include <optional>
namespace ovr::app {
class ConfigFile {
public:
    bool refresh(const Options&, bool first);
    const config::Config& current() const { return cfg_; }
private:
    config::Config cfg_;
    std::optional<std::filesystem::file_time_type> previous_;
};
}

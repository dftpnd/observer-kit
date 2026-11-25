#include "app.hpp"
#include "cli.hpp"
#include "config_file.hpp"
#include "ovr/config/config.hpp"
#include "ovr/offline/session.hpp"
#include "ovr/offsets/offset_registry.hpp"
#include <iostream>
#include <optional>
namespace ovr::app {
bool ConfigFile::refresh(const Options& options, bool first) {
    namespace fs = std::filesystem;
    std::error_code ec;
    const auto stamp = fs::last_write_time(options.config, ec);
    if (ec) {
        if (first && (options.config_required || ec != std::errc::no_such_file_or_directory)) {
            std::cerr << "config: " << ec.message() << '\n';
            return false;
        }
        if (previous_) {
            cfg_ = {};
            previous_.reset();
            std::cerr << "config unavailable; using defaults\n";
        }
    } else if (previous_ != stamp) {
        config::Config candidate;
        std::string error;
        if (config::load(options.config, candidate, error)) cfg_ = std::move(candidate);
        else {
            std::cerr << error << '\n';
            if (first) return false;
        }
        previous_ = stamp;
    }
    return true;
}

int run(int argc, char** argv) {
    Options options;
    if (!parse(argc, argv, options)) {
        usage(std::cerr);
        return 2;
    }
    if (options.help) {
        usage(std::cout);
        return 0;
    }
    if (options.build && offsets::OffsetRegistry::load_from("contracts", options.build).build() != options.build)
        return 1;

    if (options.env_requested) std::cerr << "offline: host environment unavailable\n";
    ConfigFile config;
    offline::Session session;
    for (int frame = 0; frame < options.count; ++frame) {
        if (!config.refresh(options, frame == 0)) return 2;
        const auto count = session.frame(config.current());
        std::cout << "frame=" << frame + 1 << " mode=offline primitives=" << count;
        if (options.width) std::cout << " window=" << options.width << 'x' << options.height;
        std::cout << '\n';
    }
    return 0;
}
}

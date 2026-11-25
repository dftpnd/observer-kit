#include "cli.hpp"
#include <charconv>
#include <ostream>
#include <string_view>
namespace ovr::app {
namespace {
bool positive(std::string_view s, int& out) {
    auto [end, error] = std::from_chars(s.data(), s.data() + s.size(), out);
    return error == std::errc{} && end == s.data() + s.size() && out > 0;
}
}
bool parse(int argc, char** argv, Options& out) {
    for (int i = 1; i < argc; ++i) {
        const std::string_view key = argv[i];
        if (key == "--help") {
            out.help = true;
            continue;
        }
        if (++i == argc) return false;
        const std::string_view value = argv[i];
        if (value.empty() || value.starts_with("--")) return false;
        if (key == "--config") {
            out.config = argv[i];
            out.config_required = true;
        } else if (key == "--env") {
            out.env_requested = true;
        } else if (key == "--build") {
            if (!positive(value, out.build)) return false;
        } else if (key == "--once") {
            if (!positive(value, out.count)) return false;
        } else if (key == "--window") {
            auto x = value.find('x');
            if (x == value.npos || !positive(value.substr(0, x), out.width) ||
                !positive(value.substr(x + 1), out.height)) return false;
        } else return false;
    }
    return true;
}
void usage(std::ostream& out) {
    out << "ovr [--config PATH] [--env PATH] [--build N] [--window WxH] [--once N] [--help]\n"
        << "Offline mock session. Default: one frame, optional ./config.json.\n"
        << "--build validates a local snapshot in ./contracts; --window labels the output.\n"
        << "--env is reserved for an unavailable host adapter.\n";
}
}

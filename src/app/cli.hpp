#pragma once
#include <filesystem>
#include <iosfwd>
namespace ovr::app {
struct Options {
    std::filesystem::path config{"config.json"};
    int build{}, width{}, height{}, count{1};
    bool config_required{}, env_requested{}, help{};
};
bool parse(int argc, char** argv, Options& out);
void usage(std::ostream&);
}

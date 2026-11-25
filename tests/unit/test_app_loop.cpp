#include "../../src/app/config_file.hpp"
#include <chrono>
#include <fstream>
#include <iostream>
namespace fs = std::filesystem;
int main(int, char** argv) {
    auto directory = fs::absolute(argv[0]).parent_path() / "config-reload-test";
    fs::create_directories(directory);
    ovr::app::Options options;
    options.config = directory / "config.json";
    options.config_required = true;
    ovr::app::ConfigFile file;
    auto start = fs::file_time_type::clock::now();
    auto replace = [&](const char* contents, int seconds) {
        std::ofstream(options.config) << contents;
        fs::last_write_time(options.config, start + std::chrono::seconds(seconds));
    };
    replace(R"({"marker_box_color":7})", 0);
    bool ok = file.refresh(options, true) && file.current().marker_box_color == 7;
    replace(R"({"marker_box_color":)", 5);
    ok &= file.refresh(options, false) && file.current().marker_box_color == 7;
    replace(R"({"marker_box_color":3})", 10);
    ok &= file.refresh(options, false) && file.current().marker_box_color == 3;
    fs::remove(options.config);
    ok &= file.refresh(options, false) && file.current() == ovr::config::Config{};
    fs::remove(directory);
    if (!ok) std::cerr << "config reload failed\n";
    return ok ? 0 : 1;
}

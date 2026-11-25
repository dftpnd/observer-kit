#include "ovr/config/config.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace {
using namespace ovr;
using namespace ovr::config;
namespace fs = std::filesystem;

int failures = 0;
void check(bool ok, const char* message) {
    if (!ok) {
        ++failures;
        std::cerr << message << '\n';
    }
}

void test(const fs::path& dir) {
    const fs::path fixtures = "tests/fixtures/config";
    Config c, expected;
    std::string error;
    check(load(fixtures / "good.json", c, error), "load good");
    expected.overlay_enabled = true;
    expected.combo = {true, core::Key::mouse5, 15};
    expected.last_hit_helper = {true, "#00FF00", true};
    expected.vision_tracker = {true, true, true};
    expected.camera = {1400, true};
    check(c == expected && error.empty(), "all sample fields");

    for (auto name : {"bad_type.json", "unknown_key.json", "out_of_range.json"}) {
        c = expected;
        check(!load(fixtures / name, c, error) && !error.empty() && c == Config{}, name);
    }
    c = expected;
    c.camera.custom_distance = 2500;
    check(!validate(c, error) && error == "camera.custom_distance", "camera range");
    c = expected;
    c.combo.cast_delay_ms = 250;
    check(!validate(c, error) && error == "combo.cast_delay_ms", "delay range");
    c = expected;
    c.cl_weather = 11;
    check(!validate(c, error) && error == "cl_weather", "weather range");

    const auto file = dir / "config.json";
    expected.menu_hotkey = core::Key::mouse5;
    expected.skeleton.enabled = true;
    expected.camera.enabled = true;
    expected.marker_box_color = 7;
    expected.marker_distance_units = 1.2345678f;
    check(save(file, expected) && load(file, c, error) && c == expected, "round-trip");
    check(save(file, Config{}) && load(file, c, error) && c == Config{}, "replace existing file");

    for (auto invalid : {R"({"menu_hotkey":"insert"})", R"({"menu_hotkey":"nope"})",
                         R"({"menu_hotkey":false})"}) {
        std::ofstream(file) << invalid;
        check(!load(file, c, error) && c == Config{} &&
              error.find("menu_hotkey") != std::string::npos, "key name in diagnostics");
    }
    for (auto invalid : {R"({"combo":{"cast_delay_ms":01}})", R"({"marker_distance_units":1.})",
                         R"({"overlay_enabled":true,})", R"({}null)", R"({"combo":false})",
                         R"({"camera":{},"camera":{}})", R"({"combo.cast_delay_ms":7})"}) {
        std::ofstream(file) << invalid;
        check(!load(file, c, error) && c == Config{} && !error.empty(), "invalid JSON");
    }
    std::ofstream(file) << R"({"menu\u005fhotkey":"MO\u0055SE5"})";
    check(load(file, c, error) && c.menu_hotkey == core::Key::mouse5, "escaped key");
    std::ofstream(file) << R"({"overlay_enabled":true,"camera":)";
    Config active = expected;
    if (load(file, c, error)) active = c;
    check(active == expected && c == Config{} && !error.empty(), "truncated reload");

    fs::remove(file);
    std::ostringstream captured;
    auto* previous = std::cerr.rdbuf(captured.rdbuf());
    bool deleted = true;
    for (int i = 0; i < 100; ++i) {
        deleted &= !load(file, c, error) && c == Config{} && !error.empty();
    }
    std::cerr.rdbuf(previous);
    check(deleted && captured.str().empty(), "missing file diagnostics");
}
}
int main() {
    const std::filesystem::path dir = "build/tmp/test_config";
    std::filesystem::create_directories(dir);
    test(dir);
    return failures == 0 ? 0 : 1;
}

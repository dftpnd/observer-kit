#pragma once

#include "ovr/core/key.hpp"

#include <cstdint>
#include <filesystem>
#include <string>

namespace ovr::config {

struct Config {
    bool overlay_enabled{};
    core::Key menu_hotkey{core::Key::insert};
    bool markers_enabled{true};
    std::uint32_t marker_box_color{};
    float marker_distance_units{1.f};

    struct FeatureSwitch {
        bool enabled{};
        bool operator==(const FeatureSwitch&) const = default;
    };
    FeatureSwitch markers;
    FeatureSwitch last_hit;
    FeatureSwitch timers;
    FeatureSwitch skeleton;
    FeatureSwitch snap_lines;
    FeatureSwitch fow_overlay;
    FeatureSwitch range_display;
    FeatureSwitch cooldowns;
    FeatureSwitch illusions;
    FeatureSwitch alerts;

    struct Combo {
        bool enabled{};
        core::Key target_lock_key{core::Key::mouse5};
        int cast_delay_ms{15};
        bool operator==(const Combo&) const = default;
    } combo;

    struct LastHitHelper {
        bool enabled{};
        std::string indicator_color{"#00FF00"};
        bool show_deny_threshold{};
        bool operator==(const LastHitHelper&) const = default;
    } last_hit_helper;

    struct VisionTracker {
        bool show_enemy_cooldowns{};
        bool beacon_timer_overlay{};
        bool illusion_highlight{};
        bool operator==(const VisionTracker&) const = default;
        bool enabled{};
    } vision_tracker;

    struct Camera {
        int custom_distance{1200};
        bool smooth_zoom{};
        bool operator==(const Camera&) const = default;
        bool enabled{};
    } camera;

    int cl_weather{};
    bool operator==(const Config&) const = default;
};

bool load(const std::filesystem::path&, Config& out, std::string& error);
bool save(const std::filesystem::path&, const Config&);
bool validate(const Config&, std::string& error);

}

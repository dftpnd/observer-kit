#include "ovr/config/config.hpp"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <limits>
#include <locale>
#include <set>
#include <string_view>
#include <utility>

namespace ovr::config {
namespace {


// Одна таблица определяет имена и порядок полей при чтении и записи.
template<class Configuration, class Visit>
void fields(Configuration& c, Visit visit) {
    visit("", "overlay_enabled", c.overlay_enabled);
    visit("", "menu_hotkey", c.menu_hotkey);
    visit("", "markers_enabled", c.markers_enabled);
    visit("", "marker_box_color", c.marker_box_color);
    visit("", "marker_distance_units", c.marker_distance_units);
    visit("", "cl_weather", c.cl_weather);
    visit("markers", "enabled", c.markers.enabled);
    visit("last_hit", "enabled", c.last_hit.enabled);
    visit("timers", "enabled", c.timers.enabled);
    visit("skeleton", "enabled", c.skeleton.enabled);
    visit("snap_lines", "enabled", c.snap_lines.enabled);
    visit("fow_overlay", "enabled", c.fow_overlay.enabled);
    visit("range_display", "enabled", c.range_display.enabled);
    visit("cooldowns", "enabled", c.cooldowns.enabled);
    visit("illusions", "enabled", c.illusions.enabled);
    visit("alerts", "enabled", c.alerts.enabled);
    visit("combo", "enabled", c.combo.enabled);
    visit("combo", "target_lock_key", c.combo.target_lock_key);
    visit("combo", "cast_delay_ms", c.combo.cast_delay_ms);
    visit("last_hit_helper", "enabled", c.last_hit_helper.enabled);
    visit("last_hit_helper", "indicator_color", c.last_hit_helper.indicator_color);
    visit("last_hit_helper", "show_deny_threshold", c.last_hit_helper.show_deny_threshold);
    visit("vision_tracker", "enabled", c.vision_tracker.enabled);
    visit("vision_tracker", "show_enemy_cooldowns", c.vision_tracker.show_enemy_cooldowns);
    visit("vision_tracker", "beacon_timer_overlay", c.vision_tracker.beacon_timer_overlay);
    visit("vision_tracker", "illusion_highlight", c.vision_tracker.illusion_highlight);
    visit("camera", "enabled", c.camera.enabled);
    visit("camera", "custom_distance", c.camera.custom_distance);
    visit("camera", "smooth_zoom", c.camera.smooth_zoom);
}

int hex_digit(char ch) {
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
    if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
    return -1;
}

class Reader {
public:
    explicit Reader(std::string_view input) : input_(input) {}

    bool parse(Config& c, std::string& error) {
        if (!object(c, "") || !rest().empty()) {
            error = field_.empty() ? "config: invalid JSON object" : field_ + ": invalid key or value";
            return false;
        }
        return true;
    }

private:
    std::string_view input_;
    std::string field_;

    std::string_view rest() {
        input_.remove_prefix(std::min(input_.find_first_not_of(" \t\r\n"), input_.size()));
        return input_;
    }

    bool take(std::string_view literal) {
        if (!rest().starts_with(literal)) return false;
        input_.remove_prefix(literal.size());
        return true;
    }

    // Имена и все допустимые строковые значения схемы состоят из ASCII.
    bool string(std::string& out) {
        if (!take("\"")) return false;
        out.clear();
        while (!input_.empty()) {
            char ch = input_.front();
            input_.remove_prefix(1);
            if (ch == '"') return true;
            if (ch == '\\') {
                if (input_.empty()) return false;
                ch = input_.front();
                input_.remove_prefix(1);
                if (ch == 'u' && input_.size() >= 4) {
                    unsigned code = 0;
                    for (char digit : input_.substr(0, 4)) {
                        int value = hex_digit(digit);
                        if (value < 0) return false;
                        code = code * 16 + static_cast<unsigned>(value);
                    }
                    input_.remove_prefix(4);
                    if (code > 0x7F) return false;
                    ch = static_cast<char>(code);
                } else if (ch != '"' && ch != '\\' && ch != '/') {
                    return false;
                }
            }
            if (static_cast<unsigned char>(ch) < 0x20 || static_cast<unsigned char>(ch) >= 0x7F)
                return false;
            out.push_back(ch);
        }
        return false;
    }

    bool value(std::string& s) { return string(s); }

    bool value(core::Key& key) {
        std::string name;
        if (!string(name)) return false;
        auto parsed = core::key_from_name(name);
        if (parsed) key = *parsed;
        return parsed.has_value();
    }

    bool value(bool& b) {
        if (take("true")) { b = true; return true; }
        if (take("false")) { b = false; return true; }
        return false;
    }

    template<class Number>
    bool value(Number& out) {
        const auto text = rest();
        const auto start = text.starts_with('-') ? 1u : 0u;
        if (start == text.size() || text[start] < '0' || text[start] > '9') return false;
        auto end_integer = text.find_first_not_of("0123456789", start);
        if (end_integer == std::string_view::npos) end_integer = text.size();
        if (text[start] == '0' && end_integer - start > 1) return false;
        if (end_integer < text.size() && text[end_integer] == '.' &&
            (end_integer + 1 == text.size() || text[end_integer + 1] < '0' || text[end_integer + 1] > '9')) {
            return false;
        }
        const auto [end, ec] = std::from_chars(text.data(), text.data() + text.size(), out);
        if (ec != std::errc{}) return false;
        input_.remove_prefix(static_cast<std::size_t>(end - text.data()));
        return true;
    }

    bool object(Config& c, std::string_view section) {
        if (!take("{")) return false;
        if (take("}")) return true;
        std::set<std::string> seen;
        do {
            std::string key;
            if (!string(key)) return false;
            field_ = section.empty() ? key : std::string(section) + "." + key;
            if (!seen.insert(key).second || !take(":")) return false;
            bool known_section = false;
            bool parsed = false;
            fields(c, [&](std::string_view group, std::string_view name, auto& target) {
                if (group == section && name == key) parsed = value(target);
                if (section.empty() && !group.empty() && group == key) known_section = true;
            });
            if (known_section) parsed = object(c, key);
            if (!parsed) return false;
            if (take("}")) return true;
        } while (take(","));
        return false;
    }
};

void write_value(std::ostream& out, core::Key v) { out << '"' << core::key_name(v) << '"'; }
void write_value(std::ostream& out, const std::string& v) { out << '"' << v << '"'; }
template<class Value>
void write_value(std::ostream& out, Value v) { out << v; }

bool write_file(const std::filesystem::path& file, const Config& c) {
    std::ofstream out(file, std::ios::binary);
    if (!out) return false;
    out.imbue(std::locale::classic());
    out << std::boolalpha << std::setprecision(std::numeric_limits<float>::max_digits10) << "{";
    std::string_view group;
    bool first = true;
    fields(c, [&](std::string_view next, std::string_view key, const auto& value) {
        if (next != group && !group.empty()) out << "\n  }";
        if (!first) out << ',';
        if (next != group) out << "\n  \"" << next << "\": {";
        group = next;
        out << (group.empty() ? "\n  \"" : "\n    \"") << key << "\": ";
        write_value(out, value);
        first = false;
    });
    if (!group.empty()) out << "\n  }";
    out << "\n}\n";
    out.close();
    return !out.fail();
}
}

bool validate(const Config& c, std::string& error) {
    error.clear();
    if (core::key_name(c.menu_hotkey).empty()) error = "menu_hotkey";
    else if (core::key_name(c.combo.target_lock_key).empty()) error = "combo.target_lock_key";
    else if (c.camera.custom_distance < 900 || c.camera.custom_distance > 2000) error = "camera.custom_distance";
    else if (c.combo.cast_delay_ms < 0 || c.combo.cast_delay_ms > 200) error = "combo.cast_delay_ms";
    else if (c.cl_weather < 0 || c.cl_weather > 10) error = "cl_weather";
    else if (!std::isfinite(c.marker_distance_units)) error = "marker_distance_units";
    else {
        const auto& color = c.last_hit_helper.indicator_color;
        if (color.size() != 7 || color[0] != '#' ||
            !std::all_of(color.begin() + 1, color.end(), [](char ch) { return hex_digit(ch) >= 0; })) {
            error = "last_hit_helper.indicator_color";
        }
    }
    return error.empty();
}

bool load(const std::filesystem::path& file, Config& out, std::string& error) {
    out = {};
    std::ifstream in(file, std::ios::binary);
    if (!in) { error = "config: open failed"; return false; }
    std::string data;
    char buffer[4096];
    while (in.read(buffer, sizeof(buffer)) || in.gcount() > 0)
        data.append(buffer, static_cast<std::size_t>(in.gcount()));
    if (in.bad() || !in.eof()) { error = "config: read failed"; return false; }
    Config candidate;
    if (!Reader(data).parse(candidate, error) || !validate(candidate, error)) return false;
    out = std::move(candidate);
    return true;
}

bool save(const std::filesystem::path& file, const Config& c) {
    std::string error;
    if (!validate(c, error) || file.filename().empty()) return false;
    std::error_code ec;
    // Отдельный каталог выдаёт уникальный путь даже при одновременной записи двух файлов.
    auto directory = file;
    for (unsigned i = 0; ; ++i) {
        directory = file;
        directory += ".tmp." + std::to_string(i);
        if (std::filesystem::create_directory(directory, ec)) break;
        if (ec) return false;
    }
    const auto temporary = directory / "config.json";
    bool ok = write_file(temporary, c);
    if (ok) {
        std::filesystem::rename(temporary, file, ec);
        ok = !ec;
    }
    std::filesystem::remove(temporary, ec);
    std::filesystem::remove(directory, ec);
    return ok;
}
}

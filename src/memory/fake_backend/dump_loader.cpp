#include "dump_loader.hpp"

#include <charconv>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <map>
#include <set>
#include <string_view>

namespace ovr::memory {
namespace {

void skip_space(std::string_view& input) {
    const auto first = input.find_first_not_of(" \r\n\t");
    input.remove_prefix(first == input.npos ? input.size() : first);
}

bool take(std::string_view& input, char value) {
    skip_space(input);
    if (!input.starts_with(value)) {
        return false;
    }
    input.remove_prefix(1);
    return true;
}

bool hex_quad(std::string_view& input, unsigned& value) {
    if (input.size() < 4) {
        return false;
    }
    const auto end = input.data() + 4;
    const auto result = std::from_chars(input.data(), end, value, 16);
    if (result.ec != std::errc{} || result.ptr != end) {
        return false;
    }
    input.remove_prefix(4);
    return true;
}

bool unicode_escape(std::string_view& input, std::string& out) {
    unsigned cp{};
    if (!hex_quad(input, cp)) {
        return false;
    }
    if (cp >= 0xd800 && cp <= 0xdbff) {
        if (!input.starts_with("\\u")) {
            return false;
        }
        input.remove_prefix(2);
        unsigned low{};
        if (!hex_quad(input, low) || low < 0xdc00 || low > 0xdfff) {
            return false;
        }
        cp = 0x10000 + ((cp - 0xd800) << 10) + low - 0xdc00;
    } else if (cp >= 0xdc00 && cp <= 0xdfff) {
        return false;
    }
    if (cp < 0x80) {
        out.push_back(static_cast<char>(cp));
    } else {
        const int bits = cp < 0x800 ? 6 : (cp < 0x10000 ? 12 : 18);
        const unsigned lead = bits == 6 ? 0xc0 : (bits == 12 ? 0xe0 : 0xf0);
        out.push_back(static_cast<char>(lead | (cp >> bits)));
        for (int i = bits - 6; i >= 0; i -= 6) {
            out.push_back(static_cast<char>(0x80 | ((cp >> i) & 0x3f)));
        }
    }
    return true;
}

bool parse_string(std::string_view& input, std::string& out) {
    if (!take(input, '"')) {
        return false;
    }
    out.clear();
    while (!input.empty()) {
        const char c = input.front();
        input.remove_prefix(1);
        if (c == '"') {
            return true;
        }
        if (static_cast<unsigned char>(c) < 0x20) {
            return false;
        }
        if (c != '\\') {
            out.push_back(c);
            continue;
        }
        if (input.empty()) {
            return false;
        }
        const char escaped = input.front();
        input.remove_prefix(1);
        switch (escaped) {
        case '"': case '/': case '\\': out.push_back(escaped); break;
        case 'b': out.push_back('\b'); break;
        case 'f': out.push_back('\f'); break;
        case 'n': out.push_back('\n'); break;
        case 'r': out.push_back('\r'); break;
        case 't': out.push_back('\t'); break;
        case 'u':
            if (!unicode_escape(input, out)) {
                return false;
            }
            break;
        default: return false;
        }
    }
    return false;
}

template <typename UInt>
bool parse_number(std::string_view& input, UInt& out, int radix = 10) {
    if (input.empty() || (radix == 10 && input.front() == '0' && input.size() > 1 &&
                         input[1] >= '0' && input[1] <= '9')) {
        return false;
    }
    const auto result = std::from_chars(input.data(), input.data() + input.size(), out, radix);
    if (result.ec != std::errc{}) {
        return false;
    }
    input.remove_prefix(static_cast<std::size_t>(result.ptr - input.data()));
    return true;
}

bool parse_base(std::string_view& input, std::uintptr_t& out) {
    if (!input.starts_with('"')) {
        return parse_number(input, out);
    }
    std::string value;
    if (!parse_string(input, value)) {
        return false;
    }
    std::string_view number = value;
    const bool hex = number.starts_with("0x") || number.starts_with("0X");
    if (hex) {
        number.remove_prefix(2);
    }
    return parse_number(number, out, hex ? 16 : 10) && number.empty();
}

struct Region {
    std::string module, file;
    std::uintptr_t base{};
    std::uintmax_t offset{};
    std::size_t size{};
};

bool parse_region(std::string_view& input, Region& region) {
    if (!take(input, '{')) {
        return false;
    }
    std::set<std::string> fields;
    do {
        std::string key;
        if (!parse_string(input, key) || !take(input, ':') || !fields.insert(key).second) {
            return false;
        }
        skip_space(input);
        bool ok = false;
        if (key == "module") {
            ok = parse_string(input, region.module) && !region.module.empty();
        } else if (key == "file") {
            ok = parse_string(input, region.file) && !region.file.empty() &&
                 region.file.find('\0') == std::string::npos;
        } else if (key == "base") {
            ok = parse_base(input, region.base) && region.base != 0;
        } else if (key == "file_offset") {
            ok = parse_number(input, region.offset);
        } else if (key == "size") {
            ok = parse_number(input, region.size) && region.size != 0;
        }
        if (!ok) {
            return false;
        }
    } while (take(input, ','));
    return take(input, '}') && !region.module.empty() && !region.file.empty() &&
           region.base != 0 && region.size > 0 &&
           region.size - 1 <= std::numeric_limits<std::uintptr_t>::max() - region.base;
}

}  // namespace

DumpLoadResult load_dump_map(const std::string& map_path) {
    std::ifstream map_file(map_path, std::ios::binary);
    if (!map_file) {
        return {{}, false, "нет файла карты дампа"};
    }
    const std::string text((std::istreambuf_iterator<char>(map_file)), std::istreambuf_iterator<char>());
    std::string_view input = text;
    if (!take(input, '[')) {
        return {{}, false, "ожидался массив модулей"};
    }

    std::error_code error;
    const auto dir = std::filesystem::canonical(map_path, error).parent_path();
    if (error) {
        return {{}, false, "каталог карты недоступен"};
    }
    std::map<std::filesystem::path, std::ifstream> files;
    DumpLoadResult result;
    do {
        Region region;
        if (!parse_region(input, region)) {
            return {{}, false, "некорректная запись карты"};
        }
        const auto path = std::filesystem::canonical(dir / region.file, error);
        const auto relative = path.lexically_relative(dir);
        if (error || relative.empty() || *relative.begin() == ".." ||
            !std::filesystem::is_regular_file(path, error)) {
            return {{}, false, "файл дампа должен лежать в каталоге карты"};
        }
        auto& dump = files.try_emplace(path, path, std::ios::binary).first->second;
        dump.seekg(0, std::ios::end);
        const auto length = dump.tellg();
        MapEntry entry{std::move(region.module), region.base, {}};
        if (length < 0 || region.offset > static_cast<std::uintmax_t>(length) ||
            region.size > static_cast<std::uintmax_t>(length) - region.offset ||
            region.size > static_cast<std::uintmax_t>(std::numeric_limits<std::streamsize>::max()) ||
            region.size > entry.bytes.max_size()) {
            return {{}, false, "файл дампа недоступен или диапазон за его границей"};
        }
        dump.seekg(static_cast<std::streamoff>(region.offset));
        entry.bytes.resize(region.size);
        if (!dump.read(reinterpret_cast<char*>(entry.bytes.data()), static_cast<std::streamsize>(region.size))) {
            return {{}, false, "не удалось прочитать диапазон дампа"};
        }
        result.entries.push_back(std::move(entry));
    } while (take(input, ','));

    if (!take(input, ']')) {
        return {{}, false, "карта обрывается"};
    }
    skip_space(input);
    if (!input.empty()) {
        return {{}, false, "лишние данные после карты"};
    }
    result.ok = true;
    return result;
}
}

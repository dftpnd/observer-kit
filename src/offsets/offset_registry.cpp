// Реестр смещений (T003): чтение contracts/offsets.client-<build>.json, разбор, is_stale.
// Разбор JSON здесь свой и минимальный: зависимостей кроме stdlib нет (PRINCIPLES P2).
#include "ovr/offsets/offset_registry.hpp"

#include <array>
#include <charconv>
#include <set>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

namespace ovr::offsets {
namespace {

// Потолок смещения из contracts/schemas/offsets.schema.json (число живёт в схеме, не здесь).
constexpr std::int64_t kMaxOffset = std::numeric_limits<std::int32_t>::max();

// --- SHA-256: дайджест байтов загруженного JSON (канал сверки снапшотов, contracts/README.md) ---

constexpr std::array<std::uint32_t, 64> kRoundConstants = {
    0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u, 0x3956c25bu, 0x59f111f1u, 0x923f82a4u, 0xab1c5ed5u,
    0xd807aa98u, 0x12835b01u, 0x243185beu, 0x550c7dc3u, 0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u, 0xc19bf174u,
    0xe49b69c1u, 0xefbe4786u, 0x0fc19dc6u, 0x240ca1ccu, 0x2de92c6fu, 0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau,
    0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u, 0xc6e00bf3u, 0xd5a79147u, 0x06ca6351u, 0x14292967u,
    0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu, 0x53380d13u, 0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u,
    0xa2bfe8a1u, 0xa81a664bu, 0xc24b8b70u, 0xc76c51a3u, 0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u,
    0x19a4c116u, 0x1e376c08u, 0x2748774cu, 0x34b0bcb5u, 0x391c0cb3u, 0x4ed8aa4au, 0x5b9cca4fu, 0x682e6ff3u,
    0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u, 0x90befffau, 0xa4506cebu, 0xbef9a3f7u, 0xc67178f2u,
};

constexpr std::array<std::uint32_t, 8> kSeed = {
    0x6a09e667u, 0xbb67ae85u, 0x3c6ef372u, 0xa54ff53au, 0x510e527fu, 0x9b05688cu, 0x1f83d9abu, 0x5be0cd19u,
};

std::uint32_t rotate_right(std::uint32_t value, unsigned bits) {
    return (value >> bits) | (value << (32u - bits));
}

std::string sha256_hex(std::string_view data) {
    std::array<std::uint32_t, 8> state = kSeed;
    std::vector<std::uint8_t> bytes(data.begin(), data.end());
    const std::uint64_t bit_length = static_cast<std::uint64_t>(bytes.size()) * 8u;

    bytes.push_back(0x80u);
    while (bytes.size() % 64u != 56u) {
        bytes.push_back(0u);
    }
    for (int shift = 56; shift >= 0; shift -= 8) {
        bytes.push_back(static_cast<std::uint8_t>((bit_length >> shift) & 0xFFu));
    }

    for (std::size_t block = 0; block < bytes.size(); block += 64u) {
        std::array<std::uint32_t, 64> word{};
        for (std::size_t index = 0; index < 16u; ++index) {
            const std::size_t at = block + index * 4u;
            word[index] = (static_cast<std::uint32_t>(bytes[at]) << 24u) |
                          (static_cast<std::uint32_t>(bytes[at + 1]) << 16u) |
                          (static_cast<std::uint32_t>(bytes[at + 2]) << 8u) |
                          static_cast<std::uint32_t>(bytes[at + 3]);
        }
        for (std::size_t index = 16u; index < 64u; ++index) {
            const std::uint32_t s0 = rotate_right(word[index - 15u], 7u) ^
                                     rotate_right(word[index - 15u], 18u) ^
                                     (word[index - 15u] >> 3u);
            const std::uint32_t s1 = rotate_right(word[index - 2u], 17u) ^
                                     rotate_right(word[index - 2u], 19u) ^
                                     (word[index - 2u] >> 10u);
            word[index] = word[index - 16u] + s0 + word[index - 7u] + s1;
        }

        std::array<std::uint32_t, 8> work = state;
        for (std::size_t index = 0; index < 64u; ++index) {
            const std::uint32_t s1 = rotate_right(work[4u], 6u) ^ rotate_right(work[4u], 11u) ^
                                     rotate_right(work[4u], 25u);
            const std::uint32_t choose = (work[4u] & work[5u]) ^ (~work[4u] & work[6u]);
            const std::uint32_t temp1 = work[7u] + s1 + choose + kRoundConstants[index] + word[index];
            const std::uint32_t s0 = rotate_right(work[0u], 2u) ^ rotate_right(work[0u], 13u) ^
                                     rotate_right(work[0u], 22u);
            const std::uint32_t majority = (work[0u] & work[1u]) ^ (work[0u] & work[2u]) ^
                                           (work[1u] & work[2u]);
            const std::uint32_t temp2 = s0 + majority;
            work[7u] = work[6u];
            work[6u] = work[5u];
            work[5u] = work[4u];
            work[4u] = work[3u] + temp1;
            work[3u] = work[2u];
            work[2u] = work[1u];
            work[1u] = work[0u];
            work[0u] = temp1 + temp2;
        }
        for (std::size_t index = 0; index < 8u; ++index) {
            state[index] += work[index];
        }
    }

    const auto hex_digit = [](unsigned nibble) {
        return static_cast<char>(nibble < 10u ? '0' + nibble : 'a' + nibble - 10u);
    };
    std::string hex;
    hex.reserve(64u);
    for (const std::uint32_t value : state) {
        for (int shift = 28; shift >= 0; shift -= 4) {
            hex.push_back(hex_digit(static_cast<unsigned>((value >> shift) & 0xFu)));
        }
    }
    return hex;
}

// --- минимальный разбор JSON: объект, числа, строки; всё остальное нам не встречается ---

struct Node {
    enum class Kind { object, integer } kind = Kind::object;
    std::vector<std::pair<std::string, Node>> members;
    std::int64_t integer = 0;
};

class Parser {
public:
    explicit Parser(std::string_view text) : text_(text) {}

    bool parse(Node& root) {
        skip_space();
        if (!parse_value(root, 0)) {
            return false;
        }
        skip_space();
        return position_ == text_.size();   // мусор после значения = файл битый
    }

    std::string_view reason() const { return reason_; }

private:
    bool fail(std::string_view reason) {
        reason_ = reason;
        return false;
    }

    void skip_space() {
        while (position_ < text_.size()) {
            const char symbol = text_[position_];
            if (symbol == ' ' || symbol == '\t' || symbol == '\n' || symbol == '\r') {
                ++position_;
            } else {
                break;
            }
        }
    }

    bool parse_value(Node& node, unsigned depth) {
        if (position_ == text_.size()) {
            return fail("обрыв файла");
        }
        if (text_[position_] == '{') {
            // Корень и таблица модуля — единственные объекты в схеме.
            if (depth > 1) {
                return fail("лишний уровень вложенности");
            }
            return parse_object(node, depth);
        }
        return parse_number(node);
    }

    bool parse_object(Node& node, unsigned depth) {
        std::set<std::string> keys;
        node.kind = Node::Kind::object;
        ++position_;   // '{'
        skip_space();
        if (position_ < text_.size() && text_[position_] == '}') {
            ++position_;
            return true;
        }
        while (true) {
            skip_space();
            if (position_ >= text_.size() || text_[position_] != '"') {
                return fail("ожидалось имя ключа");
            }
            std::string key;
            if (!parse_string(key)) {
                return false;
            }
            skip_space();
            if (position_ >= text_.size() || text_[position_] != ':') {
                return fail("ожидалось ':'");
            }
            ++position_;
            skip_space();
            if (!keys.insert(key).second) {
                return fail("повторяющееся имя ключа");
            }
            Node value;
            if (!parse_value(value, depth + 1)) {
                return false;
            }
            node.members.emplace_back(std::move(key), std::move(value));
            skip_space();
            if (position_ >= text_.size()) {
                return fail("обрыв объекта");
            }
            if (text_[position_] == ',') {
                ++position_;
                continue;
            }
            if (text_[position_] == '}') {
                ++position_;
                return true;
            }
            return fail("ожидался ',' или '}'");
        }
    }

    bool parse_string(std::string& out) {
        ++position_;   // '"'
        out.clear();
        while (position_ < text_.size()) {
            const char symbol = text_[position_++];
            if (symbol == '"') {
                return true;
            }
            if (symbol != '\\') {
                out.push_back(symbol);
                continue;
            }
            if (position_ >= text_.size()) {
                return fail("обрыв escape-последовательности");
            }
            const char escaped = text_[position_++];
            switch (escaped) {
            case '"': out.push_back('"'); break;
            case '\\': out.push_back('\\'); break;
            case '/': out.push_back('/'); break;
            case 'b': out.push_back('\b'); break;
            case 'f': out.push_back('\f'); break;
            case 'n': out.push_back('\n'); break;
            case 'r': out.push_back('\r'); break;
            case 't': out.push_back('\t'); break;
            case 'u': {
                if (position_ + 4 > text_.size()) {
                    return fail("обрыв \\u");
                }
                unsigned code = 0;
                for (int index = 0; index < 4; ++index) {
                    const char digit = text_[position_++];
                    code <<= 4u;
                    if (digit >= '0' && digit <= '9') {
                        code |= static_cast<unsigned>(digit - '0');
                    } else if (digit >= 'a' && digit <= 'f') {
                        code |= static_cast<unsigned>(digit - 'a' + 10);
                    } else if (digit >= 'A' && digit <= 'F') {
                        code |= static_cast<unsigned>(digit - 'A' + 10);
                    } else {
                        return fail("битый \\u");
                    }
                }
                // Имена ключей у нас ASCII: нет ASCII-символа в \u — кладём заменитель, и ключ
                // всё равно не пройдёт шаблон схемы (fail-closed, P10).
                if (code < 0x80u) {
                    out.push_back(static_cast<char>(code));
                } else {
                    out.push_back('?');
                }
                break;
            }
            default:
                return fail("неизвестный escape");
            }
        }
        return fail("незакрытая строка");
    }

    bool parse_number(Node& node) {
        const char* begin = text_.data() + position_;
        const char* end = text_.data() + text_.size();
        const auto [next, error] = std::from_chars(begin, end, node.integer);
        if (error != std::errc{}) {
            return fail("ожидалось целое число");
        }
        const char* digits = begin + (*begin == '-' ? 1 : 0);
        if (*digits == '0' && next - digits > 1) {
            return fail("ведущий ноль в числе");
        }
        position_ += static_cast<std::size_t>(next - begin);
        node.kind = Node::Kind::integer;
        return true;
    }

    std::string_view text_;
    std::size_t position_ = 0;
    std::string_view reason_ = "не разобрано";
};

bool matches_module_name(std::string_view name) {
    if (name.size() < 5 || name.substr(name.size() - 4) != ".dll") {
        return false;
    }
    for (const char symbol : name.substr(0, name.size() - 4)) {
        const bool ok = (symbol >= 'a' && symbol <= 'z') || (symbol >= '0' && symbol <= '9');
        if (!ok) {
            return false;
        }
    }
    return true;
}

bool matches_symbol_name(std::string_view name) {
    if (name.empty() || !((name.front() >= 'A' && name.front() <= 'Z') ||
                          (name.front() >= 'a' && name.front() <= 'z'))) {
        return false;
    }
    for (const char symbol : name.substr(1)) {
        const bool ok = (symbol >= 'A' && symbol <= 'Z') || (symbol >= 'a' && symbol <= 'z') ||
                        (symbol >= '0' && symbol <= '9') || symbol == '_';
        if (!ok) {
            return false;
        }
    }
    return true;
}

// RESEARCH.md §5: L2 (смещение внутри объекта, суффикс "_xxx") и L3 (статический объект)
// не разыменовываются; остальное — L1, указатель на объект, одно разыменование.
// Суффикс не различает L3 и размеры окна: эти имена §5 называет явно, список закрывает дырку.
constexpr std::string_view kNoDereferenceNames[] = {
    "dwInputSystem", "dwSoundSystem",   // L3: статический объект
    "dwWindowWidth", "dwWindowHeight",  // L2 без суффикса: значение, а не указатель
};

Deref deref_for(std::string_view symbol_name) {
    if (symbol_name.find('_') != std::string_view::npos) {
        return Deref::None;
    }
    for (const std::string_view name : kNoDereferenceNames) {
        if (name == symbol_name) {
            return Deref::None;
        }
    }
    return Deref::Once;
}

}  // namespace

OffsetRegistry OffsetRegistry::load_from(const std::string& contracts_dir, int build) {
    const std::string path =
        contracts_dir + "/offsets.client-" + std::to_string(build) + ".json";

    std::ifstream file(path, std::ios::binary);
    if (!file) {
        std::cerr << "offset-registry: " << path << ": not-established — снапшота этого билда "
                  << "нет, нужен дамп (T011)\n";
        return {};
    }
    const std::string bytes((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());
    if (file.bad()) {
        std::cerr << "offset-registry: " << path << ": not-established — чтение файла "
                  << "прервано\n";
        return {};
    }

    Node root;
    Parser parser(bytes);
    if (!parser.parse(root)) {
        std::cerr << "offset-registry: " << path << ": JSON не разобран: " << parser.reason()
                  << "\n";
        return {};
    }
    if (root.kind != Node::Kind::object || root.members.size() < 2) {
        std::cerr << "offset-registry: " << path << ": пустой снапшот или нет модулей\n";
        return {};
    }

    std::map<std::string, Symbol, std::less<>> symbols;
    bool has_version = false;
    for (const auto& [key, value] : root.members) {
        if (key == "schema_version") {
            has_version = true;
            if (value.kind != Node::Kind::integer || value.integer != 1) {
                std::cerr << "offset-registry: " << path << ": schema_version не 1\n";
                return {};
            }
            continue;
        }
        if (!matches_module_name(key)) {
            std::cerr << "offset-registry: " << path << ": ключ верхнего уровня не модуль: "
                      << key << "\n";
            return {};
        }
        if (value.kind != Node::Kind::object || value.members.empty()) {
            std::cerr << "offset-registry: " << path << ": значение модуля не объект: " << key
                      << "\n";
            return {};
        }
        for (const auto& [symbol_name, offset] : value.members) {
            if (!matches_symbol_name(symbol_name) || offset.kind != Node::Kind::integer ||
                offset.integer <= 0 || offset.integer > kMaxOffset) {
                std::cerr << "offset-registry: " << path << ": смещение негодно: " << key << "!"
                          << symbol_name << "\n";
                return {};
            }
            // stride == 0: цепочек Chain в снапшоте нет, поля m_* приходят с T011.
            symbols[key + "!" + symbol_name] =
                Symbol{static_cast<std::uintptr_t>(offset.integer), deref_for(symbol_name), 0};
        }
    }
    if (!has_version) {
        std::cerr << "offset-registry: " << path << ": отсутствует schema_version\n";
        return {};
    }
    if (symbols.empty()) {
        std::cerr << "offset-registry: " << path << ": в снапшоте нет ни одного символа\n";
        return {};
    }

    OffsetRegistry registry;
    registry.build_ = build;
    registry.sha256_hex_ = sha256_hex(bytes);
    registry.symbols_ = std::move(symbols);
    return registry;
}

std::optional<Symbol> OffsetRegistry::lookup(std::string_view name) const {
    const auto found = symbols_.find(name);
    if (found == symbols_.end()) {
        return std::nullopt;
    }
    return found->second;
}

int OffsetRegistry::build() const {
    return build_;
}

std::string_view OffsetRegistry::sha256() const {
    return sha256_hex_;
}

bool OffsetRegistry::is_stale(int current_build) const {
    return current_build != 0 && current_build != build_;
}

}

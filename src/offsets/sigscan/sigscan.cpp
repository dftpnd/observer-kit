// Скан байтовых образцов (T010): линейный поиск с пропуском по первому фиксированному байту,
// внешних библиотек нет (§10.9). Ошибка разбора — не исключение, а пустой результат (E39).
#include "ovr/offsets/sigscan/sigscan.hpp"

#include <cctype>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <vector>

namespace ovr::offsets::sigscan {
namespace {

// Байт образца: где mask == 0xFF байт зафиксирован, mask == 0 — любой, 0xF0 — старший ниббл.
struct ByteSpec {
    std::uint8_t value{};
    std::uint8_t mask{0xFF};
};

int hex_value(char symbol) {
    if (symbol >= '0' && symbol <= '9') {
        return symbol - '0';
    }
    if (symbol >= 'a' && symbol <= 'f') {
        return symbol - 'a' + 10;
    }
    if (symbol >= 'A' && symbol <= 'F') {
        return symbol - 'A' + 10;
    }
    return -1;
}

bool parse_pattern(std::string_view text, std::vector<ByteSpec>& spec, std::string_view& reason) {
    spec.clear();
    std::size_t position = 0;
    while (position < text.size()) {
        if (std::isspace(static_cast<unsigned char>(text[position]))) {
            ++position;
            continue;
        }
        std::size_t end = position;
        while (end < text.size() && !std::isspace(static_cast<unsigned char>(text[end]))) {
            ++end;
        }
        const std::string_view token = text.substr(position, end - position);
        position = end;

        if (token == "??") {
            spec.push_back(ByteSpec{0, 0});
            continue;
        }
        if (token.size() == 2 && (token.front() == '?' || token.back() == '?')) {
            // Формы `?X` и `X?` различаются позицией маски: §10.9 называет `?X` и говорит про
            // старший ниббл, а DoD требует «по старшему/младшему». Поддерживаем обе позиции,
            // иначе трактовка в Т011 разойдётся с трактовкой здесь.
            const bool low_fixed = token.front() == '?';
            const int nibble = hex_value(low_fixed ? token[1] : token[0]);
            if (nibble < 0) {
                reason = "неизвестный ниббл";
                return false;
            }
            spec.push_back(low_fixed ? ByteSpec{static_cast<std::uint8_t>(nibble), 0x0F}
                                     : ByteSpec{static_cast<std::uint8_t>(nibble << 4), 0xF0});
            continue;
        }
        if (token.size() == 2) {
            const int high = hex_value(token[0]);
            const int low = hex_value(token[1]);
            if (high < 0 || low < 0) {
                reason = "не hex-байт";
                return false;
            }
            spec.push_back(ByteSpec{static_cast<std::uint8_t>((high << 4) | low), 0xFF});
            continue;
        }
        // Одиночный "?" или «4» — не описаны в §10.9: считаем шаблон невалидным (P10).
        reason = "размер токена не 2 символа";
        return false;
    }
    return !spec.empty();
}

bool matches_at(std::span<const std::byte> image, std::size_t site, const std::vector<ByteSpec>& spec) {
    for (std::size_t index = 0; index < spec.size(); ++index) {
        const auto byte = static_cast<std::uint8_t>(image[site + index]);
        if ((byte & spec[index].mask) != spec[index].value) {
            return false;
        }
    }
    return true;
}

// x86 кладёт disp32 младшим байтом вперёд.
std::uint32_t little_endian32(std::span<const std::byte> image, std::size_t at) {
    return static_cast<std::uint32_t>(static_cast<std::uint8_t>(image[at])) |
           (static_cast<std::uint32_t>(static_cast<std::uint8_t>(image[at + 1])) << 8u) |
           (static_cast<std::uint32_t>(static_cast<std::uint8_t>(image[at + 2])) << 16u) |
           (static_cast<std::uint32_t>(static_cast<std::uint8_t>(image[at + 3])) << 24u);
}

std::uintptr_t image_base(std::span<const std::byte> image) {
    return reinterpret_cast<std::uintptr_t>(image.data());
}

}  // namespace

std::vector<Match> scan(std::span<const std::byte> image, std::span<const Pattern> patterns) {
    std::vector<Match> found;
    const std::uintptr_t base = image_base(image);

    for (const Pattern& pattern : patterns) {
        std::vector<ByteSpec> spec;
        std::string_view reason = "пустой шаблон";
        if (!parse_pattern(pattern.text, spec, reason)) {
            std::cerr << "sigscan: " << pattern.mnemonic << ": шаблон не разобран: " << reason
                      << " — not-established\n";
            continue;
        }
        if (spec.size() > image.size()) {
            std::cerr << "sigscan: " << pattern.mnemonic
                      << ": образец длиннее образа — совпадений нет\n";
            continue;
        }

        const std::size_t last = image.size() - spec.size();
        const bool anchored = spec.front().mask == 0xFF;   // пропуск по первому фиксированному байту
        const std::size_t before = found.size();
        for (std::size_t site = 0; site <= last; ++site) {
            if (anchored && static_cast<std::uint8_t>(image[site]) != spec.front().value) {
                continue;
            }
            if (matches_at(image, site, spec)) {
                found.push_back(Match{base + site, static_cast<std::uint32_t>(spec.size())});
            }
        }
        if (found.size() == before) {
            std::cerr << "sigscan: " << pattern.mnemonic << ": совпадений нет — not-established\n";
        }
    }
    return found;
}

std::optional<std::uintptr_t> rip_target(std::span<const std::byte> image, const Match& match,
                                         std::size_t disp_offset, std::size_t instruction_length) {
    if (image.empty()) {
        return std::nullopt;
    }
    const std::uintptr_t base = image_base(image);
    if (match.address < base) {
        return std::nullopt;
    }
    const std::size_t site = static_cast<std::size_t>(match.address - base);
    if (site >= image.size() || image.size() - site < 4 || image.size() - site - 4 < disp_offset) {
        return std::nullopt;   // disp32 не целиком внутри образа (E40)
    }

    const std::int32_t displacement =
        static_cast<std::int32_t>(little_endian32(image, site + disp_offset));
    // Цель обязана остаться внутри образа: за его границами адрес нами не интерпретируется (E40).
    if (instruction_length >= image.size() - site) {
        return std::nullopt;
    }
    const std::size_t next_instruction = site + instruction_length;
    // Цель = base + адрес + длина инструкции + смещение ( instruction_length один раз).
    const std::int64_t offset = static_cast<std::int64_t>(displacement);
    if (offset < 0 && static_cast<std::uint64_t>(-offset) > next_instruction) {
        return std::nullopt;   // ушли влево за начало образа
    }
    const std::uint64_t target = static_cast<std::uint64_t>(base) +
                                 (next_instruction + static_cast<std::uint64_t>(offset));
    if (target < base || target - base >= image.size()) {
        return std::nullopt;
    }
    return static_cast<std::uintptr_t>(target);
}

}

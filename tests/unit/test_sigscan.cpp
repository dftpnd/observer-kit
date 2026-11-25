// Тесты сканера образцов (T010): синтетические байты, хост и диск не нужны.
#include "ovr/offsets/sigscan/sigscan.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <vector>

using namespace ovr::offsets::sigscan;

namespace {

int failures = 0;

void check(bool condition) {
    if (!condition) {
        ++failures;
    }
}

constexpr std::size_t kImageSize = 4096;

std::vector<std::byte> make_image() {
    return std::vector<std::byte>(kImageSize, std::byte{0});
}

std::uintptr_t base_of(const std::vector<std::byte>& image) {
    return reinterpret_cast<std::uintptr_t>(image.data());
}

void place(std::vector<std::byte>& image, std::size_t at, std::initializer_list<int> bytes) {
    for (const int byte : bytes) {
        image[at++] = static_cast<std::byte>(byte);
    }
}

bool scans_clean(std::span<const std::byte> image, std::span<const Pattern> patterns,
                 std::vector<Match>& out) {
    try {
        out = scan(image, patterns);
        return true;
    } catch (...) {
        return false;
    }
}

// DoD 1: единственный вписанный образец находится ровно один раз.
void check_single_hit() {
    std::vector<std::byte> image = make_image();
    place(image, 0x400, {0x48, 0x8B, 0x05, 0x3A, 0x12, 0x00, 0x00});

    const Pattern pattern{"dwTest", "48 8B 05 3A 12 00 00"};
    std::vector<Match> found;
    check(scans_clean(std::span<const std::byte>(image), std::span<const Pattern>(&pattern, 1),
                      found));
    check(found.size() == 1);
    check(found.size() == 1 && found[0].address == base_of(image) + 0x400);
    check(found.size() == 1 && found[0].size == 7);
}

// DoD 2: `??` в середине находит оба вхождения, различающиеся только этим байтом.
void check_wildcard_middle() {
    std::vector<std::byte> image = make_image();
    place(image, 0x100, {0x48, 0x8B, 0x05, 0xC7});
    place(image, 0x900, {0x48, 0xA1, 0x05, 0xC7});
    place(image, 0x200, {0x48, 0x8B, 0x06, 0xC7});   // не совпадает: другой зафиксированный байт

    const Pattern pattern{"dwWild", "48 ?? 05 C7"};
    std::vector<Match> found;
    check(scans_clean(std::span<const std::byte>(image), std::span<const Pattern>(&pattern, 1),
                      found));
    check(found.size() == 2);
    check(found.size() == 2 && found[0].address == base_of(image) + 0x100);
    check(found.size() == 2 && found[1].address == base_of(image) + 0x900);
}

// DoD 3: каждая позиция может задавать фиксированный ниббл.
void check_high_nibble() {
    std::vector<std::byte> image = make_image();
    place(image, 0x010, {0x48, 0x8B, 0x05});
    place(image, 0x020, {0x4C, 0x8B, 0x05});
    place(image, 0x030, {0x49, 0x8B, 0x05});
    place(image, 0x040, {0x58, 0x8B, 0x05});   // другой старший ниббл -> не совпадает

    const Pattern pattern{"dwNibble", "4? 8B 05"};   // фиксирован старший ниббл
    std::vector<Match> found;
    check(scans_clean(std::span<const std::byte>(image), std::span<const Pattern>(&pattern, 1),
                      found));
    check(found.size() == 3);
    for (const Match& match : found) {
        check(match.size == 3);
    }

    const Pattern low{"dwLowNibble", "?8 8B 05"};    // фиксирован младший ниббл
    check(scans_clean(std::span<const std::byte>(image), std::span<const Pattern>(&low, 1),
                      found));
    check(found.size() == 2);   // 0x48 и 0x58, но не 0x4C и не 0x49
    check(found.size() == 2 && found[0].address == base_of(image) + 0x010);
    check(found.size() == 2 && found[1].address == base_of(image) + 0x040);

    // Толерантный разбор: лишние пробелы и нижний регистр hex не меняют результат.
    const Pattern sloppy{"dwSloppy", "  4?   8b\t05  "};
    check(scans_clean(std::span<const std::byte>(image), std::span<const Pattern>(&sloppy, 1),
                      found));
    check(found.size() == 3);

    // Несколько шаблонов -> совпадения по всем (адреса по возрастанию внутри каждого).
    const Pattern patterns[] = {{"dwNibble", "4? 8B 05"}, {"dwHead", "48 8B 05 3A"}};
    check(scans_clean(std::span<const std::byte>(image), std::span<const Pattern>(patterns, 2),
                      found));
    check(found.size() == 3);
}

// DoD 4: rip-цель считается; за границы образа уходит в nullopt (E40).
void check_rip_target() {
    std::vector<std::byte> image = make_image();
    place(image, 0x300, {0x48, 0x8B, 0x05, 0x00, 0x01, 0x00, 0x00});
    place(image, 0x500, {0x48, 0x8D, 0x0D, 0xF0, 0xFF, 0xFF, 0x7F});

    const Pattern pattern{"dwRip", "48 8B 05"};
    std::vector<Match> found;
    check(scans_clean(std::span<const std::byte>(image), std::span<const Pattern>(&pattern, 1),
                      found));
    check(found.size() == 1);

    if (found.size() != 1) {
        return;
    }
    check(!rip_target(image, found.front(), 3, std::numeric_limits<std::size_t>::max()));

    const std::optional<std::uintptr_t> target =
        rip_target(std::span<const std::byte>(image), found[0], 3, 7);
    check(target.has_value());
    check(target.has_value() && target.value() == base_of(image) + 0x300 + 7 + 0x100);

    // disp_offset за концом образа -> nullopt.
    check(!rip_target(std::span<const std::byte>(image), found[0], kImageSize, 7).has_value());

    // disp32 целиком не помещается в образ -> nullopt.
    place(image, kImageSize - 2, {0x48, 0x8B});
    const Pattern tail{"dwTail", "48 8B"};
    check(scans_clean(std::span<const std::byte>(image), std::span<const Pattern>(&tail, 1),
                      found));
    const std::vector<Match> tail_matches = found;
    bool tail_ok = true;
    for (const Match& match : tail_matches) {
        if (const std::size_t site = match.address - base_of(image); site + 4 > kImageSize) {
            tail_ok = tail_ok && !rip_target(std::span<const std::byte>(image), match, 0, 2).has_value();
        }
    }
    check(tail_ok);

    // смещение уводит цель за образ -> nullopt
    const Pattern far{"dwFar", "48 8D 0D"};
    check(scans_clean(std::span<const std::byte>(image), std::span<const Pattern>(&far, 1),
                      found));
    check(found.size() == 1);
    check(found.size() == 1 &&
          !rip_target(std::span<const std::byte>(image), found[0], 3, 7).has_value());
}

// DoD 5: пустой образ, пустой шаблон, мусорный шаблон -> пусто и без исключений (E39).
void check_degraded() {
    std::vector<std::byte> empty_image;
    const Pattern good{"dwGood", "48 8B 05"};
    const Pattern empty{"dwEmpty", ""};
    const Pattern garbage{"dwGarbage", "zz zz"};
    const Pattern lone{"dwLone", "?"};

    std::vector<Match> found;
    check(scans_clean(std::span<const std::byte>(empty_image), std::span<const Pattern>(&good, 1),
                      found));
    check(found.empty());
    check(scans_clean(std::span<const std::byte>(make_image()), std::span<const Pattern>(&empty, 1),
                      found));
    check(found.empty());
    check(scans_clean(std::span<const std::byte>(make_image()),
                      std::span<const Pattern>(&garbage, 1), found));
    check(found.empty());
    check(scans_clean(std::span<const std::byte>(make_image()), std::span<const Pattern>(&lone, 1),
                      found));
    check(found.empty());

    const std::vector<Match> match{Match{base_of(empty_image), 3}};
    try {
        check(!rip_target(std::span<const std::byte>(empty_image), match[0], 3, 7).has_value());
    } catch (...) {
        check(false);
    }
}

}  // namespace

int main() {
    check_single_hit();
    check_wildcard_middle();
    check_high_nibble();
    check_rip_target();
    check_degraded();
    return failures == 0 ? 0 : 1;
}

// Тесты реестра смещений (T003): JSON-фикстуры, хост не нужен.
#include "ovr/offsets/offset_registry.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>

using namespace ovr::offsets;

namespace {

int failures = 0;

void check(bool condition) {
    if (!condition) {
        ++failures;
    }
}

// Канал загрузки жёсткий: каталог + offsets.client-<build>.json (contracts/README.md,
// «Версионирование»), поэтому фикстуры раскладываются по этому имени в build/ (тестам
// разрешено писать только в build/).
constexpr const char* kFixtureDir = "tests/fixtures/offsets";
constexpr const char* kScratch = "build/tmp/test_offset_registry";

bool stage(const std::string& dir, std::string_view fixture, int build) {
    std::error_code error;
    std::filesystem::create_directories(dir, error);
    if (error) {
        return false;
    }
    std::filesystem::copy_file(std::string(kFixtureDir) + "/" + std::string(fixture),
                               dir + "/offsets.client-" + std::to_string(build) + ".json",
                               std::filesystem::copy_options::overwrite_existing, error);
    return !error;
}

bool same_symbol(const std::optional<Symbol>& left, const std::optional<Symbol>& right) {
    if (left.has_value() != right.has_value()) {
        return false;
    }
    return !left.has_value() || (left->rva == right->rva && left->deref == right->deref &&
                                 left->stride == right->stride);
}

bool empty_registry(const OffsetRegistry& registry) {
    return registry.build() == 0 && registry.sha256().empty() &&
           !registry.lookup("client.dll!dwGameEntitySystem").has_value() &&
           !registry.lookup("engine2.dll!dwNetworkGameClient").has_value();
}

void check_snapshot() {
    const std::string dir = std::string(kScratch) + "/valid";
    check(stage(dir, "valid.json", 6985));

    bool thrown = false;
    OffsetRegistry registry;
    try {
        registry = OffsetRegistry::load_from(dir, 6985);
    } catch (...) {
        thrown = true;
    }
    check(!thrown);   // бросать запрещено (E2, E30)

    check(registry.build() == 6985);
    check(registry.sha256() ==
          "f7edb57a44f4a4bb4ab12dc7665045b23f14976cb4a93c3cd26beb1d042c14e4");
    check(registry.sha256().size() == 64);

    const auto entity_system = registry.lookup("client.dll!dwGameEntitySystem");
    check(entity_system.has_value());
    check(entity_system->rva == 108484944);
    check(entity_system->deref == Deref::Once);   // L1: указатель на объект (RESEARCH.md §5)

    const auto entity_count = registry.lookup("client.dll!dwGameEntitySystem_entityCount");
    check(entity_count.has_value());
    check(entity_count->rva == 8384);
    check(entity_count->deref == Deref::None);    // L2: смещение внутри объекта

    const auto delta_tick = registry.lookup("engine2.dll!dwNetworkGameClient_deltaTick");
    check(delta_tick.has_value());
    check(delta_tick->rva == 588);

    check(!registry.lookup("client.dll!dwViewMatrix_static").has_value());   // удалён в 6985
    check(!registry.lookup("client.dll!dwGameEntitySystem").value().stride);  // цепочек нет

    // L3 и L2 без суффикса: разыменования нет (RESEARCH.md §5).
    check(registry.lookup("inputsystem.dll!dwInputSystem")->deref == Deref::None);
    check(registry.lookup("soundsystem.dll!dwSoundSystem")->deref == Deref::None);
    check(registry.lookup("engine2.dll!dwWindowWidth")->deref == Deref::None);

    // Алиасы живут двумя ключами, как в JSON (contracts/README.md, «Алиасы»).
    check(same_symbol(registry.lookup("client.dll!dwEntityList"),
                      registry.lookup("client.dll!dwGameEntitySystem")));
    check(same_symbol(registry.lookup("engine2.dll!dwNetworkGameClient_serverTickCount"),
                      registry.lookup("engine2.dll!dwNetworkGameClient_deltaTick")));
}

void check_stale() {
    // 6900-снапшот лежит в каталоге, где нет 6985: брать чужие числа нельзя -> пустой реестр (E30).
    const std::string dir = std::string(kScratch) + "/stale";
    check(stage(dir, "stale.json", 6900));
    check(empty_registry(OffsetRegistry::load_from(dir, 6985)));

    const OffsetRegistry old_snapshot = OffsetRegistry::load_from(dir, 6900);
    check(old_snapshot.build() == 6900);
    check(!old_snapshot.is_stale(6900));
    check(old_snapshot.is_stale(6985));   // триггер re-dump (риск R3)
    check(!old_snapshot.is_stale(0));     // билд хоста неизвестен -> не считаем устаревшим
}

void check_broken() {
    const std::string broken = std::string(kScratch) + "/broken";
    check(stage(broken, "malformed.json", 6985));   // текст вместо числа (E2)
    const std::string empty = std::string(kScratch) + "/empty";
    check(stage(empty, "empty.json", 6985));

    for (const char* damaged : {
             R"({"a.dll":{"X":1},"b.dll":{"Y":2}})",
             R"({"schema_version":1,"a.dll":{"X":+1}})",
             R"({"schema_version":1,"a.dll":{"X":01}})",
             R"({"schema_version":1,"a.dll":{"X":1,"X":2}})",
             R"({"schema_version":1,"a.dll":{"X":{"nested":1}}})"}) {
        const std::string dir = std::string(kScratch) + "/invalid-input";
        std::filesystem::create_directories(dir);
        std::ofstream(dir + "/offsets.client-6985.json") << damaged;
        check(empty_registry(OffsetRegistry::load_from(dir, 6985)));
    }
    bool thrown = false;
    try {
        check(empty_registry(OffsetRegistry::load_from(broken, 6985)));
        check(empty_registry(OffsetRegistry::load_from(empty, 6985)));
        check(empty_registry(OffsetRegistry::load_from(std::string(kScratch) + "/absent", 6985)));
    } catch (...) {
        thrown = true;
    }
    check(!thrown);
}

void check_copy_semantics() {
    const std::string dir = std::string(kScratch) + "/valid";
    const OffsetRegistry original = OffsetRegistry::load_from(dir, 6985);
    OffsetRegistry copy = original;
    check(copy.build() == original.build());
    check(copy.sha256() == original.sha256());
    check(same_symbol(copy.lookup("client.dll!dwGameEntitySystem"),
                      original.lookup("client.dll!dwGameEntitySystem")));

    // Публичных мутаторов нет: изменённый снапшот = новый load_from, а не правка реестра.
    copy = OffsetRegistry{};
    check(copy.build() == 0);
    check(original.lookup("client.dll!dwGameEntitySystem").has_value());
}

}  // namespace

int main() {
    check_snapshot();
    check_stale();
    check_broken();
    check_copy_semantics();
    return failures == 0 ? 0 : 1;
}

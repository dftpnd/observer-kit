# TASKS — 35 задач, каждая самостоятельна

Документ разбит так, чтобы одну задачу можно было скопировать целиком в чистую worktree
сильной модели, у которой **нет знания хоста и нет знания остальных задач**. Всё нужное
задача говорит прочитать; всё, что задача обязана вернуть, — зафиксировано сигнатурами.

## Как делегировать

- **Один fenced-блок = одна worktree = один исполнитель.** Никогда не отдавай две задачи
  одному исполнителю: они могут пересечься файлами (см. `ARCHITECTURE.md:419`).
- Порядок задаётся milestone'ами ниже и таблицей зависимостей в конце. Задачи внутри
  одного milestone'а без пересечения зависимостей можно выполнять параллельно.
- Исполнитель получает: текст задачи + право читать файлы из поля «Вход» + свою worktree.
  Он **не получает** ни `RESEARCH.md`, ни `ARCHITECTURE.md` целиком — только указанные строки.
- Приёмка любой задачи: `cmake --build build && ctest --test-dir build --output-on-failure`
  на Linux (`ARCHITECTURE.md:584`) + 5 пунктов DoD (`ARCHITECTURE.md:628`).
- Если контракта не хватает — исполнитель **не додумывает**, а возвращает «контракт неполон»
  и предлагает минимальную правку заголовка комментарием в PR. Правку делает ревьюер.

## Общие правила (действуют на все задачи, в блоках не повторяются)

- Язык C++20, стиль как в уже существующем коде репозитория, comments минимальны.
- Критерии и спорные решения — `PRINCIPLES.md` (P1–P14). Если блок задачи им противоречит,
  отвечает `PRINCIPLES.md`, а исполнитель возвращает «контракт неполон», а не додумывает.
- **Никаких перехватов, правок чужого кода, подсадки модулей** — наблюдатель внешний (`ARCHITECTURE.md:501` R1).
- **Никаких литералов смещений `>= 100000`** в `src/` и `include/` — только
  `OffsetRegistry::lookup()` (`ARCHITECTURE.md:175`). Проверка: `scripts/check_no_hardcoded_offsets.py`.
- **Никаких процессов кроме одного.** Никаких сокетов между модулями (`ARCHITECTURE.md:11`).
- Никаких `TODO`/`FIXME` вместо реализации. Неизвестное поле = значение помечается
  `not-established` в описании результата, а не захардкожено.
- Тестам запрещено: спать, читать `/proc`/реестр, зависеть от порядка запуска, требовать
  GPU/монитора/интернета, оставлять файлы вне `build/` (`ARCHITECTURE.md:584`).
- `git init`/`git commit`/`git push` в этой работе не делать: коммиты остаются за заказчиком.
- Единственный разрешённый общий файл — `src/main.cpp`: два крюка по одной строке —
  `registry.add(...)` в `register_features()` — это все фичи — и `convars::apply_from_config`
  (`cfg, backend, writer, offsets`, сигнатура §10.8, строку добавляет T022). «Ровно одна строка» значит «на одну больше, чем было
  до начала задачи»: сколько фич смержено, столько таких строк в файле. Конфликт при merge
  разрешает тот, кто мержит позже; больше в `main.cpp` менять ничего нельзя.
- Терминология: тексты, идентификаторы, строки вывода, имена файлов и ключи конфига — только
  слова из `TERMS.md`; внешние названия (репозитории, API, convar-ы) — в `бэктиках` (P12).
- Контракты — только `ARCHITECTURE.md:109` (§3) и `ARCHITECTURE.md:671` (§10).
  Задача цитирует подпись дословно и не объявляет своих типов и сигнатур, которых там нет:
  не хватает — ответ «контракт неполон» (P10), а не новая строчка в чужом заголовке.
- `Крайние случаи:` задачи — это строки каталога ARCHITECTURE.md:999 (§11). Каждый случай обязан
  быть назван в пункте DoD вот так: `E28: <что проверяем>` — случай без имени в DoD = задача не
  принята (считает `check_docs.py`); случая извне каталога в задаче быть не может (P10).
- Каждое число (ёмкость, диапазон, порог, размер пачки) уже имеет владельца в
  ARCHITECTURE.md:955 (§10.15). Новая константа в задаче без строки в той таблице — находка ревью.
- Пока `scripts/check_includes.py` (T023) не сделан, направление `#include` проверяется вручную
  по §4.2 (ARCHITECTURE.md:444). В DoD это пишется ссылкой на это правило, а не пересказом.

---

# M0 — каркас, на котором всё можно собирать

### T000 — основа дерева, CMake, зелёная сборка

```text
ID: T000
Milestone: M0. Зависимости: нет.
Цель: создать пустой, но собирающийся каркас проекта, чтобы каждая следующая задача
      клала файлы в уже существующие директории и сразу попадала в ctest.

Вход (прочитать обязательно):
  - ARCHITECTURE.md:41   — дерево каталогов и правило «включать только соседний снизу слой»
  - ARCHITECTURE.md:611  — платформы, зависимости, команда Linux-сборки
  - PRINCIPLES P14 и ARCHITECTURE.md §9 — среда хоста: ручки, генерация host_env.hpp, fail-closed

Выход (создать ровно эти файлы, ничего лишнего):
  - CMakeLists.txt — (project ovr, CXX_STANDARD 20, опция OVR_BUILD_TESTS; на каждый модуль и
    на каждую фичу строка `if(EXISTS "<dir>/CMakeLists.txt") add_subdirectory(<dir>)`, список
    фиксируется сейчас: core, offsets, memory/rpm_backend, memory/vm_readv_backend,
    memory/fake_backend, state, render, overlay, input, config, convars, ipc и
    features/{vision,last_hit,timers,skeleton,snap_lines,fow_overlay,range_display,cooldowns,
    illusions,alerts,combo}. Больше этот файл никто не трогает (PRINCIPLES P5))
  - include/ovr/core/.gitkeep, include/ovr/offsets/.gitkeep, include/ovr/memory/.gitkeep,
    include/ovr/state/.gitkeep, include/ovr/render/.gitkeep, include/ovr/overlay/.gitkeep,
    include/ovr/input/.gitkeep, include/ovr/config/.gitkeep, include/ovr/features/.gitkeep,
    include/ovr/ipc/.gitkeep
  - src/core/, src/offsets/, src/memory/, src/state/, src/render/, src/overlay/,
    src/input/, src/config/, src/convars/, src/features/, src/ipc/   (по .gitkeep)
  - src/main.cpp            — int main() { return 0; }  (тело заменяется в M2/M4)
  - tests/unit/CMakeLists.txt, tests/unit/smoke.cpp
  - tests/fixtures/offsets/.gitkeep, tests/golden/.gitkeep
  - tests/fixtures/host_env.sample.json — (нейтральная фикстура среды хоста: `sample.bin`,
    `sample/ver.inf`, `cv_a`, `cv_b`, `cv_c`, `patch_x`, `src-A`; настоящих имён хоста в ней нет)
  - scripts/gen_host_env.py — (читает `OVR_HOST_ENV` и пишет `build/generated/ovr/host_env.hpp`;
    stdlib, без зависимостей; аргументы `--env <файл>`, `--out <файл>`)
  - third_party/.gitkeep, build/.gitkeep

Контракт:
  - исполняемый target `ovr` собирается из src/main.cpp и библиотеки `ovr_lib`
  - `ovr_lib` — статическая библиотека, её include-директория `include/`
  - smoke.cpp: TEST-case, который всегда проходит (проверяет, что ctest запускается)
  - опция OVR_BUILD_TESTS=ON включает tests/; OFF — собирает только ovr
  - никаких third_party-зависимостей на этом этапе (ImGui подключается в T014)
  - `build/generated/ovr/host_env.hpp` (public include `ovr/host_env.hpp`):
    `namespace ovr::host_env` c `constexpr std::string_view kTarget, kPatchCurrent, kToolchain,
    kHostBin, kHostBinLinux, kVersionFile, kCvCameraDistance, kCvRangeDisplay, kCvParticleFow;`
    и `bool is_available()`; генерация на configure (единственное место, где настоящее имя хоста
    попадает в сборку — P14); корневой CMakeLists вызывает скрипт, больше никто его не правит (P5)
  - нет `OVR_HOST_ENV` и нет `--env` -> заглушка с пустыми значениями и `is_available() == false`
    (fail-closed, P10); в CI используется фикстура, поэтому сборка не зависит от наличия хоста

Крайние случаи: E1 (каталог ARCHITECTURE.md:999 §11).
Ограничения:
  - Внешних зависимостей нет вообще: только CMake, компилятор и stdlib Python (P2, KISS);
    `third_party/` появляется не раньше T014.
  - Корневой `CMakeLists.txt` после этой задачи заморожен: новая цель живит только внутри своей
    директории (P5), поэтому список подключений фиксируется сейчас через `if(EXISTS ...)`.
  - `gen_host_env.py` пишет только в `build/`: настоящее имя хоста не попадает ни в stdout,
    ни в дерево репозитория (P14).
  - Логики наблюдателя, потоков и сна здесь нет: задача про сборку, а не про поведение (P1).
DoD / тест:
  1) mkdir build && cmake -S . -B build -DOVR_BUILD_TESTS=ON && cmake --build build
  2) ctest --test-dir build --output-on-failure  → зелёно
  3) те же команды с -DOVR_BUILD_TESTS=OFF       → сборка без тестов тоже зелёная
  4) `tree -L 2 include src tests` совпадает с ARCHITECTURE.md:41 (с учётом .gitkeep)
  5) python3 scripts/gen_host_env.py --env tests/fixtures/host_env.sample.json ->
     в заголовке значения фикстуры (`sample.bin`), сборка зелёная
  6) E1: тот же скрипт без `--env` и без `OVR_HOST_ENV` -> `is_available() == false`, пустые
     значения, exit 0 и сборка зелёная (хоста может не быть)

Не трогать: RESEARCH.md, ARCHITECTURE.md, contracts/, TASKS.md, README.md.
Заметки: `.gitkeep` кладёт только T000; владелец директории заменяет их своими файлами
         в своей задаче и имеет право удалить .gitkeep своей директории.
         Корневой CMakeLists.txt после T000 не меняет ни одна задача: новая цель появляется
         только внутри своей директории, корень на неё не правят (PRINCIPLES P5).
```

### T001 — `core/`: типы, векторы, время

```text
ID: T001
Milestone: M0. Зависимости: T000.
Цель:
  Layer 0 — типы, которые видят все остальные слои. Больше никто ниже `core/` не лежит.

Вход:
  - ARCHITECTURE.md:114   — §3.1, точная сигнатура (копировать буква в букву)
  - ARCHITECTURE.md:126   — §3.1.1, vec.hpp и time.hpp: арифметика, проекции и часы (копировать буква в букву)
  - ARCHITECTURE.md:770   — §10.5, единый `core::Key` (копировать буква в букву)
  - ARCHITECTURE.md:41    — правило include-графов

Выход:
  - include/ovr/core/types.hpp
  - include/ovr/core/vec.hpp
  - include/ovr/core/time.hpp
  - include/ovr/core/key.hpp
  - src/core/key.cpp
  - src/core/vec.cpp
  - tests/unit/test_core.cpp

Контракт (из ARCHITECTURE.md:114, ARCHITECTURE.md:126 и ARCHITECTURE.md:770):
  namespace ovr::core
    struct Vec2 { float x{}, y{}; };
    struct Vec3 { float x{}, y{}, z{}; };
    struct Mat4x4 { float m[16]{}; };   // row-major «как кладёт Source2»
    using ModuleId = std::uint8_t;
    constexpr std::uint64_t kInvalidAddr = 0;
  vec.hpp: operator+ - * (скаляр), dot, length, length_sq, normalize, lerp;
           Mat4x4 operator*(Mat4x4), transform_point(Mat4x4, Vec3) -> Vec3 (с делением на w),
           transform_dir(Mat4x4, Vec3) -> Vec3 (без деления на w)
  time.hpp: using Clock = std::chrono::steady_clock; now_ns(), ms_to_frames(double, double hz)
  key.hpp (дословно из §10.5):
    enum class Key : std::uint16_t { unknown = 0 /* VK-коды Windows, коды Linux; MOUSE5 == 5 */ };
    std::optional<Key> key_from_name(std::string_view);   // "INSERT" -> Key, "MOUSE5" -> Key
    std::string_view key_name(Key);                        // для save() и для меню
  Таблица «имя ↔ код» живёт в src/core/key.cpp: она одна на проект (T019, T021 только читают).
  В enum вносятся те коды, которые названы в RESEARCH.md:245; своих догадок нет (P10).

Крайние случаи: E43 (каталог ARCHITECTURE.md:999 §11).
Ограничения: только <cstdint>, <array>, <cmath>, <chrono>, <string_view>, <optional>.
  Никаких <windows.h>, <string> в types.hpp; <string> разрешён лишь в src/core/key.cpp.
  Таблица клавиш — статические данные, ни одной аллокации при lookup.
DoD / тест (tests/unit/test_core.cpp, без хоста и без файлов):
  - Mat4x4{} == все нули; toждественная матрица задаётся явным конструктором identity()
  - E43: transform_point(identity, {1,2,3}) == {1,2,3}; при w<=0 возвращает {0,0,0} и не падает
  - operator* Mat4x4: M*I == M, ассоциативность на 3 матрицах с допуском 1e-5
  - E43: normalize({0,0,0}) не даёт NaN; lerp с NaN в a, b или t тоже отвечает без NaN и inf
  - static_assert(sizeof(Mat4x4) == 64)
  - key_from_name("INSERT") == Key из таблицы; key_from_name("insert") и key_from_name("nope")
    дают nullopt; key_name(key_from_name(X)) == X для всех X таблицы (round-trip без исключений)
  - ни одного литерала имени клавиши вне src/core/key.cpp (`grep -rn '"INSERT"\|"MOUSE5"' include src` —
    только key.cpp)
Не трогать: всё кроме списка выше. В частности — никаких include из offsets/memory/state.
```

### T002 — JSON-схемы контрактов + валидатор

```text
ID: T002
Milestone: M0. Зависимости: T000.
Цель: превратить contracts/*.json из «текста» в формально валидируемый контракт, чтобы
      битый дампер не ронял наблюдатель.

Вход:
  - contracts/README.md:6, :16, :37, :44, :53, :64  — формат снапшота, алиасы, версионирование, валидация
  - contracts/offsets.client-6985.json               — настоящий пример (он валиден)
  - contracts/info.client-6985.json                  — настоящий пример
  - RESEARCH.md:302   — §11: контракт как генерируемый документ

Выход:
  - contracts/schemas/offsets.schema.json
  - contracts/schemas/info.schema.json
  - scripts/check_contracts.py — (валидатор по схемам + режим --manifest: сверка
    contracts/MANIFEST.sha256 по RESEARCH.md:115; exit 0/1, вывод path:строка)
  - tests/fixtures/contracts/valid/   (копии обоих реальных JSON)
  - tests/fixtures/contracts/invalid/ (>=5 битых вариантов: нет schema_version; build не int;
       смещение отрицательное; смещение > 0x7FFFFFFF; пустой список модулей)

Контракт:
  - offsets.schema.json: Draft 2020-12; root object; schema_version (const 1) обязателен; любой
      другой ключ = имя модуля (pattern ^[a-z0-9]+\.dll$), значение = object<string, integer 0..0x7FFFFFFF>;
      минимум один модуль. Здесь нет build/generated_at/sha256: версия живёт в info.client-<build>.json,
      провенанс — в contracts/MANIFEST.sha256 (PRINCIPLES P8). Формат = реальный снапшот
      contracts/offsets.client-6985.json (RESEARCH.md:197), а не «плоский список symbols»
  - info.schema.json: обязательные schema_version, target (const "host"), platform, arch, renderer,
      patch, build (integer >= 0), source_revision, toolchain, version_source
      (enum ["version_file","server","launcher_api"]); ключи только snake_case (в реальном снапшоте
      ровно эти имена); значения-идентичности = ручки (pattern ^[a-z0-9][a-z0-9_.-]*$) либо числа
  - removed_in_<build> / not_established / contradicted — опциональные object/array, тип фиксирован
  - scripts/check_contracts.py: режим `check <файл> <схема>` и режим `diff <файл_a> <файл_b>`
      (выводит добавленные/удалённые/изменённые символы и печатает `removed:` для удалённых);
      exit 0 = ок, exit 1 = ошибка валидации, exit 2 = uso/нет файла; зависимостей кроме stdlib нет
  - diff должен печатать `ALIAS-OK` для пар-алиасов из contracts/README.md:37
      (dwEntityList==dwGameEntitySystem, deltaTick==serverTickCount) — это не регрессия

Крайние случаи: E2, E3, E31 (каталог ARCHITECTURE.md:999 §11).
Ограничения:
  - Валидатор — stdlib Python, без `jsonschema` и без сети: схема проверяется кодом, а не «на глаз» (P2).
  - `contracts/*.json` — данные: эта задача их читает и ни разу не правит, владелец — T011 (P7).
  - Схема описывает формат, а не смысл: «смещение похоже на правду» проверяет T023, не здесь (низкая связность).
  - Рядом со схемами не появляется C++-типов: валидация контрактов живёт в `scripts/`, а не в заголовках (KISS).
DoD / тест:
  - python3 scripts/check_contracts.py contracts/offsets.client-6985.json contracts/schemas/offsets.schema.json → exit 0
  - E2: python3 scripts/check_contracts.py <каждый invalid/*> → exit 1 (перебрать в shell-цикле)
  - E31: python3 scripts/check_contracts.py diff contracts/offsets.client-6985.json tests/fixtures/contracts/valid/offsets.client-6985.json → diff пустой, exit 0
  - E3: файла фикстуры нет → exit 2 и строка `not-established` со списком того, что надо добыть;
    выдуманных чисел в выводе нет (P10)
Не трогать: contracts/offsets.client-6985.json, contracts/info.client-6985.json (данные; их правит только T011).
           В contracts/README.md можно дописать только раздел «Схемы» (не переписывать остальное).
```

### T004 — интерфейс `IMemoryBackend` (только заголовок)

```text
ID: T004
Milestone: M0. Зависимости: T001.
Цель: единая точка входа в чужую память. Всё, что зависит от ОС, закрывается здесь;
      три реализации дают T005, T006, T006W (и будущий pcileech/DMA).

Вход:
  - ARCHITECTURE.md:154  — §3.2, точная сигнатура
  - ARCHITECTURE.md:679  — §10.1 `RawFrame` (тоже заголовок этой задачи)
  - ARCHITECTURE.md:787  — §10.6 `IMemoryWriter` (тоже заголовок этой задачи)
  - ARCHITECTURE.md:955  — §10.15: `kMaxReadRequests` объявляется здесь
  - RESEARCH.md:283      — §10: Windows RPM vs Linux process_vm_readv
  - ARCHITECTURE.md:456  — §5: почему батч, а не 400 отдельных read()

Выход:
  - include/ovr/memory/memory_backend.hpp    (header-only интерфейс; .cpp нет)
  - include/ovr/memory/raw_frame.hpp         (header-only view; .cpp нет)
  - include/ovr/memory/memory_writer.hpp     (header-only интерфейс; .cpp нет)

Контракт (из ARCHITECTURE.md:154, ARCHITECTURE.md:679 и ARCHITECTURE.md:787):
  namespace ovr::memory
    struct ReadResult { const std::uint8_t* data{}; std::uint32_t size{}; bool ok{}; };
    struct ReadReq    { std::uintptr_t addr; void* dst; std::uint32_t size; };
    class IMemoryBackend {
    public:
      virtual ~IMemoryBackend() = default;
      virtual bool attach(std::uint32_t pid) = 0;
      virtual std::uintptr_t module_base(std::string_view module) const = 0;
      virtual bool read_raw(std::uintptr_t addr, void* dst, std::uint32_t size) = 0;
      virtual void read_batch(std::span<const ReadReq> reqs) = 0;      // один syscall на пачку
    };
    struct RawFrame { ... }            // дословно §10.1 (ARCHITECTURE.md:679)
    struct WriteReq  { std::uintptr_t addr; const void* src; std::uint32_t size; };
    struct WriteResult { std::uint32_t written; bool ok; };
    class IMemoryWriter {
    public:
      virtual ~IMemoryWriter() = default;
      virtual bool write_raw(std::uintptr_t addr, const void* src, std::uint32_t size) = 0;
      virtual void write_batch(std::span<const WriteReq>, std::span<WriteResult> out) = 0;
    };
  constexpr std::uint32_t kMaxReadRequests = 256;   // §10.15; больше — это находка ревью
  Комментарий в заголовке обязан фиксировать семантику: read_batch НЕ бросает исключений,
  при провале чтения dst остаётся нетронутым; module_base возвращает 0, если модуль не найден.
  write_batch семантически == N вызовов write_raw; dst-буфер вызывающий, аллокаций внутри нет;
  провал одной записи не роняет всю пачку (частичный ok по факту). Ни один из трёх заголовков
  не знает про ОС и про хоста: ОС-специфика только в реализациях (T006, T006W, T005).

Крайние случаи: E6, E48 (каталог ARCHITECTURE.md:999 §11).
Ограничения: в заголовках разрешены <cstdint>, <string_view>, <span>. Никаких windows.h / sys/uio.h.
DoD / тест: каждый из трёх заголовков компилируется сам по себе (TU, включающий его дважды);
  ни одна из строк `#include <windows.h>`/`<sys/uio.h>`/`<dlfcn.h>` в этих файлах отсутствует
  (проверяется grep в DoD-скрипте задачи);
  `grep -c 'class IMemory' include/ovr/memory/*.hpp` == 1 на файл (интерфейс в одном месте);
  `grep -n kMaxReadRequests include/ovr/memory/memory_backend.hpp` ровно одна строка.
  E6: прочитано меньше байт, чем запрашивали -> в комментарии заголовка зафиксирован частичный
     отказ по флагам, пачка не уронена, `RawFrame::complete == false`, исключений нет.
  E48: в комментарии к `read_batch` зафиксировано поведение пачки длиннее `kMaxReadRequests`
    (лишние диапазоны ok==false, UB и аллокаций нет); тест компиляции собирает пачку из 300 ReadReq.

Не трогать: src/memory/** — реализаций здесь не делать, это T005/T006/T006W.
```

### T005 — `FakeBackend`: память из файла-дампа

```text
ID: T005
Milestone: M0. Зависимости: T001, T004.
Цель: читаемая «память хоста» из фикстур, чтобы 90% тестов M2+ проходили на машине БЕЗ хоста
      и без Windows. Fake обязан появиться раньше живого бэкенда.

Вход:
  - ARCHITECTURE.md:154  — §3.2 (реализуемый интерфейс)
  - ARCHITECTURE.md:787  — §10.6 `IMemoryWriter`: Fake обязан реализовать и запись (иначе T022
    не проверяет идемпотентность без хоста)
  - ARCHITECTURE.md:584  — §8: Fake раньше живого, тесты без хоста
  - ARCHITECTURE.md:456  — §5: одна пачка read_batch() за кадр

Выход:
  - src/memory/fake_backend/fake_backend.hpp
  - src/memory/fake_backend/fake_backend.cpp
  - src/memory/fake_backend/dump_loader.hpp, dump_loader.cpp
  - tests/fixtures/dumps/minimal.bin, tests/fixtures/dumps/minimal.map.json
  - tests/unit/test_fake_backend.cpp

Контракт:
  - class FakeBackend final : public memory::IMemoryBackend, public memory::IMemoryWriter
  - строится из dump-файла: load(path) → адресная карта [{base, module, bytes}]
    minimal.map.json: [{ "module": "client.dll", "base": "0x180000000", "file": "minimal.bin", "file_offset": 0, "size": 4096 }, ...]
  - attach(pid) игнорирует pid и возвращает true, если load прошёл
  - module_base("client.dll") == base из карты; неизвестный модуль → 0
  - read_raw: полное попадание в карту → ok=true; выход за границу → ok=false и dst НЕ трогается
  - read_batch: семантически == N вызовов read_raw, но internal counter batch_calls++
    (тест проверяет batch_calls == 1 на пачке из 400 запросов)
  - write_raw / write_batch: пишут в тот же буфер карты; счётчики write_calls и write_bytes++
    (T022 проверяет идемпотентность именно по write_calls); запись за границей карты → ok=false,
    буфер не тронут; write_batch семантически == N вызовов write_raw
  - тестовая подмена set_fail_range(addr, size): чтения внутри диапазона проваливаются (для тестов валидности)
  - тестовая подмена set_write_fail_range(addr, size): записи внутри диапазона проваливаются
  - карта байтов — изменяемая (const у read-интерфейса это не ломает: read отдаёт span-вид)

Крайние случаи: E7, E48 (каталог ARCHITECTURE.md:999 §11).
Ограничения:
  - `FakeBackend` не знает ни про смещения, ни про фичи: вход — адрес, размер и файл дампа,
    это одна ответственность и низкая связность.
  - Внутри `read_batch()` нет аллокаций сверх фиксированного пула слотов: счётчик аллокаций —
    часть DoD, потому что всё остальное живёт в бюджете кадра (§5).
  - Заголовки бэкенда создаёт T004: здесь только реализация и тесты, чужие директории не трогаем (P7).
  - Файл дампа читается один раз при `open()`; повторного `open()` внутри `read_batch()` нет (E7).
DoD / тест:
  1) 500 разных read_raw дают тот же результат, что и один read_batch из 500 ReadReq;
  2) E7: чтение за концом карты -> ok=false, буфер не изменён (заполнить 0xAA и сравнить),
     ОС не трогается;
  3) `module_base("inputsystem.dll")` и `module_base("nope.dll")` -> 0 и !=0 по карте;
  4) `set_fail_range` -> ok=false без исключений; ни одного вызова open() внутри read_batch;
  5) запись 4 байт и чтение их обратно даёт те же байты, `write_calls == 1`;
  6) запись за концом карты -> ok=false, соседние байты не изменены (заполнить 0xAA и сравнить);
  7) `write_batch` из 100 WriteReq -> `write_calls == 100`, `batch_calls` не увеличился;
  8) E48: пачка из kMaxReadRequests+1 ReadReq -> аллокаций нет, лишний диапазон ok==false,
     остальные прочитаны (это находка ревью, а не неопределённое поведение);
  9) `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j &&
     ctest --test-dir build --output-on-failure --tests-regex fake_backend` — зелёные на Linux.

Не трогать: src/memory/rpm_backend/** (это T006), include/ovr/memory/memory_backend.hpp,
include/ovr/memory/raw_frame.hpp, include/ovr/memory/memory_writer.hpp.
```

---

# M1 — Смещения: реестр, скан сигнатур, дампер полей

Ключевой milestone: без него весь остальной наблюдатель — гадание на адресах. Порядок внутри
milestone'а строго `T003 → T010 → T011`: реестр определяет, *что* нужно найти, сканер —
*как* найти, дампер добывает то и другое из живого хоста и единственный пишет в `contracts/`.

### T003 — OffsetRegistry: JSON → символы, проверка устаревания

```text
ID: T003
Milestone: M1. Зависимости: T001, T002.
Цель:
  Закрыть доступ к любым адресам, кроме как через реестр, и научить наблюдатель честно
  говорить «мой набор смещений не для этого билда хоста», а не читать мусор.
Вход:
  - ARCHITECTURE.md:175 — раздел 3.3, точные сигнатуры (цитата ниже), читать как данность.
  - RESEARCH.md:27 — таблица смещений build 6985 и формат ключа "<module>!<symbol>".
  - RESEARCH.md:302 — откуда берётся ClientVersion (ручка `version_file`, имя — в `HOST-ENV.json`).
  - contracts/README.md:16 — формат и версионирование JSON-контрактов смещений.
  - contracts/offsets.client-6985.json — реальные данные (28 строк), главный фикстур-образец.
  - include/ovr/core/ — то, что сделал T001; использовать, не переопределяя.
  - scripts/check_contracts.py — валидатор из T002; им же проверять свои фикстуры.
Выход:
  - include/ovr/offsets/offset_registry.hpp — (заголовок ровно по 3.3, ничего сверх)
  - src/offsets/offset_registry.cpp — (чтение contracts/*.json, разбор, is_stale)
  - src/offsets/CMakeLists.txt — (цель ovr_offsets: линкуется только к ovr_core)
  - tests/unit/test_offset_registry.cpp — (тесты на JSON-фикстурах, хост не нужна)
  - tests/fixtures/offsets/valid.json — (копия contracts/offsets.client-6985.json)
  - tests/fixtures/offsets/stale.json — (тот же JSON, но build=6900)
  - tests/fixtures/offsets/malformed.json — (битый JSON: обещанный ключ с текстом вместо числа)
  - tests/fixtures/offsets/empty.json — (пустой объект)
Контракт (из ARCHITECTURE.md:175):
  namespace ovr::offsets
    enum class Deref : std::uint8_t { None, Once, Chain };
    struct Symbol { std::uintptr_t rva; Deref deref; std::uint16_t stride; };
    class OffsetRegistry {
     public:
      static OffsetRegistry load_from(const std::string& contracts_dir, int build);
      std::optional<Symbol> lookup(std::string_view name) const;
      int build() const;
      std::string_view sha256() const;
      bool is_stale(int current_build) const;
    };
  Ключ lookup() — строка "<module>!<symbol>" ровно как в RESEARCH.md:27, регистр модуля как в JSON.
  load_from(contracts_dir, build) берёт файл offsets.client-<build>.json из contracts_dir; бросать
  запрещено — при любой ошибке (нет файла, битый JSON, неизвестный ключ схемы) возвращается
  пустой реестр: build()==0, sha256() пустой, lookup()==nullopt на любое имя; причина — одна
  строка в stderr. sha256() — hex-дайджест байтов загруженного JSON (канон: файл как есть).
  is_stale(cur) == (cur != 0 && cur != build()). Реестр копируемый и после load_from неизменяемый.
Крайние случаи: E2, E4, E30, E31 (каталог ARCHITECTURE.md:999 §11).
Ограничения:
  Никаких литералов `>= 100000` в src/ и include/ (сами числа живут только в JSON); в тестах
  числа писать можно. Никаких std::cerr кроме одной строки в load_from. Никакого кэша на диске.
DoD / тест:
  1) lookup("client.dll!dwGameEntitySystem") → rva==108484944, deref==Once;
  2) lookup("client.dll!dwGameEntitySystem_entityCount") → rva==8384, deref==None;
  3) lookup("engine2.dll!dwNetworkGameClient_deltaTick") → rva==588;
  4) lookup("client.dll!dwViewMatrix_static") → nullopt (в 6985 удалён, RESEARCH.md:27);
  5) E4: valid.json + build 6985 → is_stale(6985)==false, is_stale(6986)==true, is_stale(0)==false;
  6) E30: stale.json + build 6985 → пустой реестр; E2: malformed.json и empty.json → пустой реестр,
     без исключений (обернуть в try/catch и проверить, что catch не сработал);
  7) E31: diff снапшота, где `dwEntityList` описан вторым именем той же сущности, печатает
     `ALIAS-OK` и exit 0 (пары — contracts/README.md:37);
  8) sha256() совпадает с `sha256sum contracts/offsets.client-6985.json | cut -c1-64`;
  9) `python3 scripts/check_contracts.py tests/fixtures/offsets/valid.json` — exit 0;
  10) `cmake --build build && ctest --test-dir build -R offset_registry --output-on-failure`.
Не трогать: contracts/** (только читать), src/offsets/sigscan/** (это T010), include/ovr/core/**.
```

### T010 — скан паттернов по дампу образа

```text
ID: T010
Milestone: M1. Зависимости: T001, T003.
Цель:
  Дать дамперу (T011) инструмент найти символ в образе client.dll по байтовому образцу там,
  где готового смещения из сообщества нет; в рантайме наблюдателя сканер не участвует вообще.
Вход:
  - ARCHITECTURE.md:175 — формат Symbol/Deref (цель скана в итоге отдаётся в том же виде).
  - ARCHITECTURE.md:854 — §10.9, заголовок сканера (цитата ниже дословно).
  - RESEARCH.md:100 — методология: поле принимается только при >=2 независимых источниках.
  - ARCHITECTURE.md:501 — риск R10 (тот же критерий, формулировка для комментариев).
  - include/ovr/offsets/offset_registry.hpp — заголовок из T003 (читать как данность).
  - RESEARCH.md:72 — где берутся дампы образов и как устроены IDA-подобные записи.
Выход:
  - include/ovr/offsets/sigscan/sigscan.hpp — (подректорий, чтобы не пересекаться с T003)
  - src/offsets/sigscan/sigscan.cpp — (разбор шаблона и поиск)
  - src/offsets/sigscan/CMakeLists.txt — (цель ovr_sigscan, линкуется только к ovr_core)
  - tests/unit/test_sigscan.cpp — (синтетические байты, без хоста и без диска)
Контракт (из ARCHITECTURE.md:175):
  Сканера в §3 нет — его заголовок зафиксирован в ARCHITECTURE.md:854 (§10.9), цитата ниже
  дословно (§13.3). Форма:
    namespace ovr::offsets::sigscan
      struct Pattern { std::string_view mnemonic; std::string_view text; };
      struct Match { std::uintptr_t address; std::uint32_t size; };
      std::vector<Match> scan(std::span<const std::byte> image, std::span<const Pattern> patterns);
      std::optional<std::uintptr_t> rip_target(std::span<const std::byte> image,
                                               const Match&, std::size_t disp_offset,
                                               std::size_t instruction_length);
  Pattern.text — «48 8B 05 ?? ?? ?? ??»: байты через пробел, `??` = любой байт, `?X` = старший
  ниббл; разбор толерантный (повторные пробелы, регистр hex), невалидный шаблон => 0 совпадений
  и одна строка в stderr. scan() возвращает ВСЕ совпадения по всем шаблонам (адрес = base + индекс).
  rip_target() считает цель rip-относительного адреса: base + address + instruction_length
  + int32 из disp_offset; выход за image => nullopt.
Крайние случаи: E39, E40 (каталог ARCHITECTURE.md:999 §11).
Ограничения:
  Вызывать scan() разрешено только из tools/dumper/** (T011); в src/state/**, src/features/**,
  src/render/** упоминаний sigscan быть не должно. Скан линейный + пропуск по первому
  фиксированному байту, никаких внешних библиотек.
DoD / тест:
  1) образ 4 КБ с вписанным «48 8B 05 3A 12 00 00» → scan() даёт ровно 1 Match с ожидаемым адресом;
  2) шаблон с `??` в середине находит оба различающихся по этому байту вхождения (2 Match);
  3) шаблон «48 ?? 8B» находит по старшему/младшему нибблу по правилу `?X`;
  4) E40: rip_target() на известном шаблоне даёт ожидаемый адрес; disp_offset за концом образа → nullopt;
  5) E39: пустой image, пустой шаблон, мусор «zz zz» → пустой результат, без исключений;
  6) `grep -rn "sigscan" src/state src/features src/render include/ovr/state` → пусто;
  7) `cmake --build build && ctest --test-dir build -R sigscan --output-on-failure`.
Не трогать: tools/dumper/** (это T011), include/ovr/offsets/offset_registry.hpp, src/offsets/*.cpp.
```

### T011 — дампер: `contracts/fields.client-6985.json` (блокер M3/M4)

```text
ID: T011
Milestone: M1. Зависимости: T003, T010.
Цель:
  Добыть то, чего нет в таблицах сообщества: сетевые поля `m_*` акторов/пажей и цепочку
  камеры `dwViewRender` -> view-объект -> матрица/FOV/near/far. Без этого метки, оверлеи
  и фича добивания не принимаются (RESEARCH.md:183, риск R4).
Вход:
  - ARCHITECTURE.md:175 — во что должны превратиться добытые смещения (Symbol/Deref/stride).
  - ARCHITECTURE.md:194 — как эти смещения будет есть TypedReader (Once vs Chain, stride).
  - RESEARCH.md:183 — список полей, которые в 6985 НЕ установлены (это и есть цель задачи).
  - RESEARCH.md:100 — критерий принятия поля: >=2 независимых источника или собственный дамп.
  - RESEARCH.md:127 — устройство entity list и сетевых слотов (шаг 24 байта, highest index).
  - contracts/README.md:16 — формат контракта; contracts/README.md:37 — алиасы и версии.
  - contracts/offsets.client-6985.json, contracts/info.client-6985.json — существующие данные.
  - include/ovr/offsets/sigscan/sigscan.hpp — заголовок из T010 (использовать, не переписывать).
  - ARCHITECTURE.md:890 — §10.11: CLI `ovr_dumper`, аргументы и коды возврата (цитата ниже).
  - scripts/required_symbols.txt — если ещё нет, создать по списку ниже (это файл T011).
Выход:
  - tools/dumper/main.cpp — (CLI-обёртка: запускает внешний Rust-дампер, забирает его
    client.dll.json, нормализует и пишет наш контракт; свой парсер Source2 не переписывать, P2)
  - tools/dumper/CMakeLists.txt — (цель ovr_dumper; только линкует ovr_core + ovr_sigscan)
  - tools/dumper/README.md — (как получить дамп и как запустить, 3 команды, без воды)
  - tools/dumper/netclass.cpp — (запасной путь: точечный разбор NetworkStrings только для тех
    классов, которых не дал внешний дампер; полный свой парсер Source2 не писать — P2)
  - tools/dumper/view_chain.cpp — (dwViewRender -> view-объект -> матрица, FOV, znear, zfar)
  - contracts/fields.client-6985.json — (РЕЗУЛЬТАТ: добытые поля с evidence на каждое)
  - contracts/evidence/client-6985/ — (сырые дампы строк/хексов, доказывающие каждое поле)
  - contracts/MANIFEST.sha256 — (sha256 на fields/offsets/info и на evidence; RESEARCH.md:115, P8)
  - scripts/required_symbols.txt — (минимально необходимый список; без него CI не проверяет)
Контракт (из ARCHITECTURE.md:175):
  Формат contracts/fields.client-6985.json — как у offsets.client-6985.json (ARCHITECTURE.md:175),
  но каждый элемент обязателен с полями rva, deref, stride (0 если не Chain) и evidence: массивом
  из >=2 строк {"source": "...", "note": "..."} по RESEARCH.md:100. Поля, которые добыть не удалось,
  записываются с rva==0, deref=="None" и evidence:[{"source":"not-established","note":"<почему>"}] —
  это валидный результат, а не ошибка. Требуемый минимум ключей:
    client.dll!m_hPlayer, m_iTeamNum, m_iHealth, m_iMaxHealth, m_vecAbsOrigin,
    m_hUnit, m_iLevel, m_lifeState, m_bStunned (актор/агент);
    client.dll!dwViewRender -> view_matrix, view_fov, view_znear, view_zfar (цепочка, stride).
  CLI зафиксирован в ARCHITECTURE.md:890 (§10.11), цитата дословно:
    ovr_dumper [--build <n>] [--out <dir>] [--env <path>] [--dry-run] [--help]
    exit 0 = контракт записан либо честно not-established;
    exit 1 = внешний дампер не запустился (нет процесса, нет прав, хост не запущен);
    exit 2 = битые аргументы или повреждённый входной файл
  Имя файла результата всегда `<вид>.client-<build>.json`; без `--env` и без `OVR_HOST_ENV`
  дампер fail-closed (§9): ничего не пишет и уходит с exit 1.
Крайние случаи: E3, E36 (каталог ARCHITECTURE.md:999 §11).
Ограничения:
  T011 — ЕДИНСТВЕННАЯ задача, которой разрешено писать в contracts/** (ARCHITECTURE.md:444);
  правки RESEARCH.md и ARCHITECTURE.md делает ревьюер, не исполнитель. Дампер живёт только в
  tools/** и не линкуется в читаемый бинарник. Запуск с живым хостом допустим ТОЛЬКО в дампере и
  в ad-hoc прогоне, в ctest его быть не должно. Если живого хоста в окружении нет — не выдумывать:
  вернуть контракт с not-established и список того, что нужно добыть руками.
DoD / тест:
  1) `python3 scripts/check_contracts.py contracts/fields.client-6985.json` → exit 0;
  2) E36: `grep -c not-established contracts/fields.client-6985.json` посчитан и объяснён в tools/dumper/README.md;
  3) OffsetRegistry из T003, расширенный этим файлом, возвращает lookup("client.dll!m_vecAbsOrigin")
     с ненулевым rva (или явно not-established — но тогда это зафиксировано в DoD-отчёте);
  4) E3: живого хоста нет (исполнитель работает без дампа) -> дампер печатает not-established и список того, что надо добыть, и не пишет файл с выдуманными числами (P10);
  5) `ctest --test-dir build --output-on-failure` зелёные БЕЗ запуска хоста (дампер в тестах не запускается);
  6) `cmake --build build` собирает ovr_dumper; `./build/tools/dumper/ovr_dumper --help` печатает справку по запуску;
  7) `./build/tools/dumper/ovr_dumper --help` завершается 0; `--dry-run` не создаёт и не меняет
      ни одного файла (сверить mtime/список); без `--env` и без `OVR_HOST_ENV` — exit 1 и ни
      одного записанного файла;
  8) ни один файл за пределами «Выход» не изменён (проверить `git status`-эквивалентом: спискомdiff).
  9) contracts/MANIFEST.sha256 покрывает contracts/fields.client-6985.json, sha256sum сходится (P8).
Не трогать: RESEARCH.md, ARCHITECTURE.md, contracts/offsets.client-6985.json (читать), src/**, include/**.
```

---

# M2 — Чтение памяти, конфиг, бюджет кадра

Здесь наблюдатель впервые начинает укладываться в 1.8 мс на кадр (`ARCHITECTURE.md:456`) и перестаёт
требовать перекомпиляции ради настройки. `T015` — единственная задача с числовым DoD: если
бюджет не выдержан, дальше идти нельзя, потому что все фичи M5–M6 добавляют время, а не убирают.

### T006 — TypedReader + бэкенд Windows (ReadProcessMemory)

```text
ID: T006
Milestone: M2. Зависимости: T001, T003, T004, T005.
Цель:
  Научиться читать память хоста типизированно и пачкой: вызов идёт один на всю пачку, а
  разыменование цепочек делает сам читатель, возвращая флаг валидности вместо исключения.
Вход:
  - ARCHITECTURE.md:194 — раздел 3.4, точные сигнатуры (цитата ниже).
  - ARCHITECTURE.md:154 — §3.2 `IMemoryBackend`: контракт read_batch(), который задал T004; запись — §10.6 (ARCHITECTURE.md:787).
  - include/ovr/memory/memory_backend.hpp — заголовок T004 (читать как данность).
  - include/ovr/offsets/offset_registry.hpp + ARCHITECTURE.md:175 — Symbol/Deref/Chain, stride.
  - src/memory/fake_backend/** — поведение-эталон из T005; живой бэкенд обязан дать те же тесты.
  - RESEARCH.md:283 — почему пакетное чтение обязательно (стоимость одного syscall).
  - ARCHITECTURE.md:456 — §5: один read_batch() на кадр, <= 0.6 мс.
Выход:
  - include/ovr/memory/typed_reader.hpp — (заголовок ровно по 3.4)
  - src/memory/typed_reader.cpp — (разбор цепочек Once/Chain поверх read_batch)
  - src/memory/rpm_backend/read_process_memory_backend.cpp — (класс RpmBackend: `IMemoryBackend`)
  - src/memory/rpm_backend/writer.cpp — (тот же класс, `IMemoryWriter` из §10.6; `WriteProcessMemory` ровно один раз)
  - src/memory/rpm_backend/CMakeLists.txt — (условная компиляция: WIN32 только)
  - tests/unit/test_typed_reader.cpp — (тесты поверх FakeBackend из T005)
  - tests/unit/test_rpm_backend.cpp — (бэкенд-контракт; на Linux пропускается по платформе)
Контракт (из ARCHITECTURE.md:194):
  namespace ovr::memory
    struct ChainStep { Symbol sym; };            // deref==Once => разыменование, Chain => stride
    template<class T> struct Typed { T value{}; bool ok{}; };
    class TypedReader {
     public:
      template<class T>
      Typed<T> read(const offsets::OffsetRegistry&, std::string_view root_symbol,
                    std::span<const ChainStep> chain = {}) const;
    };
  Семантика: базовый адрес модуля берётся из backend->module_base(); нулевой или отрицательный
  адрес на любом шаге => ok==false и value{} обнулён (не изменённый мусор запрещён); неизвестный
  root_symbol => ok==false; Исключений наружу не выпускать вообще. Тип T — тривиальный, размер
  <= 64 байта (static_assert). Цепочка короче, чем требует Symbol.stride, — это ok==false, а не UB.
  RpmBackend реализует IMemoryBackend из ARCHITECTURE.md:154 (§3.2) ровно в одном методе read_batch()
  и IMemoryWriter из ARCHITECTURE.md:787 (§10.6) ровно в одном методе write_batch():
  одна пачка iovec-подобных запросов -> ОДИН вызов ReadProcessMemory на непрерывный кусок,
  провал одного диапазона не должен ронять всю пачку (частичный ok по флагам).
Крайние случаи: E5, E6, E7, E29, E48 (каталог ARCHITECTURE.md:999 §11).
Ограничения:
  В рантайме не вызывать ReadProcessMemory по одному слову (400 вызовов = 3-5 мс потерь,
  RESEARCH.md:283). WriteProcessMemory разрешён ровно в src/memory/rpm_backend/writer.cpp
  (§10.6, ARCHITECTURE.md:787) и нигде больше; VirtualAllocEx, CreateRemoteThread, LoadLibrary
  в src/memory/** запрещены совсем: подсадка модулей нет (RESEARCH.md:47, R1). Пачка длиннее
  kMaxReadRequests не аллоцируется: лишние диапазоны получают ok==false (и это находка ревью).
DoD / тест:
  1) на FakeBackend: чтение m_hPlayer-подобной цепочки Once->Once->None даёт ожидаемое значение;
  2) E5: цепочка с нулём на втором шаге => ok==false и value==T{} (заполнить value до 0x5A и проверить обнуление);
  3) E7: чтение за концом карты (FakeBackend set_fail_range) => ok==false, без исключений;
  4) unknown root_symbol => ok==false;
  5) E6: обрыв пачки в середине -> флаги по факту: у обрезанных диапазонов ok==false, остальные прочитаны, аллокаций нет;
  6) E29: бэкенд чужой платформы — файл целиком под `#ifdef`, тест пропускается по платформе, а не падает;
  7) `grep -c ReadProcessMemory src/memory/rpm_backend/read_process_memory_backend.cpp` == 1;
  8) `grep -rln WriteProcessMemory src/memory` == ровно src/memory/rpm_backend/writer.cpp,
     и `grep -c WriteProcessMemory src/memory/rpm_backend/writer.cpp` == 1;
  9) `grep -rn "VirtualAllocEx\|CreateRemoteThread\|LoadLibrary" src/memory` → пусто;
  10) `grep -rn "WriteProcessMemory" src/state src/features src/render` → пусто (писателя у фичи нет);
  11) E48: тест на пачку из kMaxReadRequests+44 запросов: аллокаций ноль, хвост ok==false;
  12) `cmake --build build && ctest --test-dir build -R "typed_reader|rpm_backend" --output-on-failure`.
Не трогать: include/ovr/memory/** (это T004), src/memory/fake_backend/** (это T005),
src/state/** (это T007/T008).
```

### T006W — бэкенд Linux: `process_vm_readv`

```text
ID: T006W
Milestone: M2. Зависимости: T001, T004, T005, T006.
Цель:
  Дать Linux-сборке настоящий бэкенд чтения, где вся пачка диапазонов уходит за один syscall —
  иначе на Linux бюджет §5 недостижим, а вся приёмка (`ARCHITECTURE.md:584`) идёт на Linux.
Вход:
  - ARCHITECTURE.md:154 — §3.2 `IMemoryBackend` (тот же интерфейс, что реализовал T004); запись — §10.6 (ARCHITECTURE.md:787).
  - ARCHITECTURE.md:194 — 3.4 (читать, чтобы не сломать ожидания TypedReader к флагам ok).
  - include/ovr/memory/memory_backend.hpp — заголовок T004, менять нельзя.
  - src/memory/rpm_backend/read_process_memory_backend.cpp — реализация-образец от T006.
  - RESEARCH.md:283 — сравнение process_vm_readv / /proc/pid/mem и выбор варианта.
  - ARCHITECTURE.md:456 — §5: <= 0.6 мс на всю пачку чтения.
Выход:
  - src/memory/vm_readv_backend/process_vm_readv_backend.cpp — (класс ProcessVmReadvBackend)
  - src/memory/vm_readv_backend/writer.cpp — (тот же класс, `IMemoryWriter` из §10.6;
    `process_vm_writev` ровно один раз и только здесь)
  - src/memory/vm_readv_backend/CMakeLists.txt — (своя цель ovr_memory_vmreadv, линкует ovr_core)
  - tests/unit/test_process_vm_readv_backend.cpp — (бэкенд-контракт на своём же процессе)
Контракт (из ARCHITECTURE.md:154 и ARCHITECTURE.md:787):
  Класс реализует IMemoryBackend из §3.2 и IMemoryWriter из §10.6 без изменения заголовков. read_batch(reqs) делает
  ровно ОДИН вызов process_vm_readv(2) со всеми iovec'ами: remote iovec по одному на диапазон,
  local iovec в предоставленный caller-буфер. short read (возврат < запрошенного) трактуется как
  частичный успех: флаги ok выставляются по фактически прочитанным байтам, хвост диапазона не
  трогать. Возврат -1 с EAGAIN/EINTR — ретраить не более 3 раз, после чего все диапазоны ok==false.
  module_base(name) — по карте загруженных модулей процесса (чтение один раз при attach, кэш).
  Никакого чтения /proc/<pid>/mem в рантайме (медленнее и рвёт пакетность, RESEARCH.md:283).
Крайние случаи: E29, E48 (каталог ARCHITECTURE.md:999 §11).
Ограничения:
  Тест обязан читать память СОБСТВЕННОГО процесса (статический массив-образец) — хост в тесте
  не нужна, /proc чужого процесса в тестах запрещён. Своя директория и свой CMakeLists.txt —
  единственное изменение сборки: общего файла с T006 больше нет (PRINCIPLES P7).
  Писатель (§10.6) пишется в отдельный writer.cpp той же цели: `ptrace` не используется никогда,
  пачка длиннее kMaxReadRequests не аллоцируется (лишние диапазоны ok==false).
DoD / тест:
  1) тест объявляет `static const std::array<std::byte, 4096> kSample`, делает из него local iovec
     и читает 8 диапазонов у собственного pid → все ok, байты совпадают с образцом;
  2) заведомо неверный адрес (0x1) в середине пачки → этот диапазон ok==false, соседние ok==true;
  3) счётчик вызовов: в тесте проверяется, что реализация использует ровно один syscall на пачку
     (`grep -c process_vm_readv src/memory/vm_readv_backend/process_vm_readv_backend.cpp` == 1);
  4) E29: сборка не-Linux без `process_vm_readv` — файл целиком под `#ifdef`, тест пропускается по платформе, а не падает;
  5) E48: пачка из kMaxReadRequests+9 диапазонов -> аллокаций нет, лишние ok==false, остальные прочитаны;
  6) тесты T005 (FakeBackend) и T006 (typed_reader) остаются зелёными;
  7) запись 8 байт в собственный процесс через writer.cpp и чтение их обратно дают те же байты;
     `grep -rln process_vm_writev src/memory` == ровно writer.cpp этой задачи;
  8) `grep -rn "ptrace\|/proc/" src/memory/vm_readv_backend` → пусто;
  9) `cmake --build build && ctest --test-dir build --output-on-failure` — полный прогон зелёный.
Не трогать: include/ovr/memory/** (это T004), read_process_memory_backend.cpp (это T006), typed_reader.*.
```

### T021 — конфиг: JSON + схема + hot-reload между кадрами

```text
ID: T021
Milestone: M2. Зависимости: T001, T002.
Цель:
  Выгнать все настраиваемые значения из кода в валидируемый файл, чтобы M5–M6 добавляли фичи
  без правки типов конфига, а кривой конфиг не ронял наблюдатель, а откатывался к дефолтам.
Вход:
  - ARCHITECTURE.md:336 — раздел 3.10, точные сигнатуры (цитата ниже).
  - RESEARCH.md:218 — образец конфига пользователя: ключи берутся ровно оттуда, новых не выдумывать.
  - RESEARCH.md:160 — список convar'ов и их легальные диапазоны (для camera/weather).
  - ARCHITECTURE.md:501 — риск R8 (cl_weather только 0-10, experimental).
  - contracts/README.md:53 — правила валидации контрактов через JSON-схемы (тот же подход).
  - scripts/check_contracts.py — валидатор из T002; схему конфигa проверять им же.
Выход:
  - include/ovr/config/config.hpp — (заголовок ровно по 3.10)
  - src/config/config.cpp — (load/save/validate)
  - src/config/CMakeLists.txt — (цель ovr_config: только ovr_core)
  - config.schema.json — (JSON Schema ключей из RESEARCH.md:245 + заранее объявленные неймспейсы
    всех фич (P6): markers, last_hit, timers, skeleton, snap_lines, fow_overlay, range_display,
    cooldowns, illusions, alerts, combo, camera, vision_tracker, last_hit_helper — каждой
    минимум {enabled, color, threshold} со своими min/max; фичи M5/M6 этот файл не правят)
  - tests/unit/test_config.cpp — (загрузка/откат/hot-reload, файл пишет в build/)
  - tests/fixtures/config/good.json, bad_type.json, unknown_key.json, out_of_range.json
Контракт (из ARCHITECTURE.md:336):
  namespace ovr::config
    struct Config { bool overlay_enabled{}; core::Key menu_hotkey{}; /* combo, last_hit_helper,
                                            vision_tracker, camera */ };
    bool load(const std::filesystem::path&, Config& out, std::string& error);
    bool save(const std::filesystem::path&, const Config&);
    bool validate(const Config&, std::string& error);
  Поля вложенных секций — ровно по образцу RESEARCH.md:218 (combo.enabled,
  last_hit_helper.enabled, vision_tracker.*, camera.custom_distance, input.* и т.д.); порядок
  объявления по секциям образца. Семантика: load() сначала применяет ВСЕ дефолты, затем парсит;
  любой сбой парсинга/типа => out == дефолты, error заполнена, возврат false; НЕИЗВЕСТНЫЙ ключ
  => ошибка загрузки (не игнор). validate() — единственное место диапазонов:
  camera.custom_distance 900..2000, input.cast_delay_ms 0..200, convar cl_weather 0..10;
  нарушение диапазона => false + текст ошибки с именем ключа. save() пишет валидный конфиг,
  который load() прочитает без ошибок (round-trip). Hot-reload: вызывающий код дёргает load()
  между кадрами; в самой загрузке мьютексов и блокировок быть не должно.
  Клавиши в JSON — строки (`"menu_hotkey": "INSERT"`, RESEARCH.md:245): перевод делает
  `core::key_from_name`/`key_name` (T001, §10.5), у конфига своей таблицы клавиш нет.
Крайние случаи: E17, E18, E19 (каталог ARCHITECTURE.md:999 §11).
Ограничения:
  Тип `Key` — только `core::Key` из ARCHITECTURE.md:770 (§10.5, его объявляет T001): в
  `config.hpp` никакого собственного `enum class Key` и никаких заголовков `include/ovr/input/`
  (это T019). Имя клавиши в JSON — строка, перевод делает `key_from_name` (T001); своих таблиц
  клавиш у конфига нет (§12.3).
  Никаких переменных окружения, реестра, `%APPDATA%`; путь передаётся аргументом.
DoD / тест:
  1) good.json → load()==true, поля совпадают с RESEARCH.md:218 (сверяется поле за полем);
  2) bad_type.json, unknown_key.json, out_of_range.json → load()==false, error непустая,
     все поля out == дефолтам (не частичное состояние!);
  3) round-trip: save() затем load() в тот же путь → Config побайтово равен (operator== в тесте);
  4) validate() ловит custom_distance==2500, cast_delay_ms==250, cl_weather==11;
  5) round-trip клавиши: good.json с `"menu_hotkey": "MOUSE5"` -> load даёт соответствующий
     `core::Key`, save пишет то же имя; `"insert"` (нижний регистр) и `"nope"` -> load==false,
      все поля == дефолтам, в error имя ключа (E17);
  6) E18: конфиг подменён между кадрами: load() на файл, записанный наполовину (обрезанный
     JSON) -> false, а `out` остаётся ровно прошлым снимком; полу-конфиг в кадр не попадает,
     потому что перезагрузка происходит только между вызовами цикла;
  7) E19: файл конфига удалили после запуска: load() на несуществующий путь -> false и все
     поля == дефолтам; 100 кадров подряд с мёртвым путём -> не больше одной строки в stderr
     про этот файл (не по строке за кадр), наблюдатель не падает;
  8) `python3 scripts/check_contracts.py config.schema.json` → exit 0;
  9) `grep -rn "getenv\|_NSGet\|HKEY" src/config` → пусто;
  10) `cmake --build build && ctest --test-dir build -R config --output-on-failure`.
Не трогать: include/ovr/input/**, src/input/** (это T019), contracts/** , scripts/check_contracts.py.
```

### T015 — SHM-снапшоты и бенчмарк бюджета кадра

```text
ID: T015
Milestone: M3. Зависимости: T001, T005, T006W, T007, T008, T021.
Цель:
  Замерить бюджет кадра ДО появления фич и завести ring буфер снапшотов, чтобы позже можно
  было отделить «наблюдатель» и «рендер» без сокетов и без второго обязательного процесса.
Вход:
  - ARCHITECTURE.md:538 — раздел 7.2, точные сигнатуры (цитата ниже).
  - ARCHITECTURE.md:456 — §5: все числовые лимиты и правила no-alloc (главный DoD задачи).
  - ARCHITECTURE.md:526 — 7.1: почему по умолчанию механизм выключен в M0-M7.
  - RESEARCH.md:259 — 9: требования к окну/рендере (потребитель снапшотов появится в T014).
  - ARCHITECTURE.md:955 (§10.15) — `kSnapshotSlots` объявляется в заголовке этой задачи.
  - include/ovr/state/host_state.hpp (T007) и src/state/parse_frame/** (T008) — читать как
    данность: сериализуются ровно их поля, своих типов эта задача не объявляет (§13.1).
  - src/memory/fake_backend/** — источник предсказуемых данных для бенчмарка.
Выход:
  - include/ovr/ipc/snapshot_stream.hpp — (заголовок ровно по 7.2)
  - src/ipc/snapshot_stream.cpp — (POSIX shm + SPSC ring на `kSnapshotSlots` слотов, CRC32)
  - src/ipc/CMakeLists.txt — (цель ovr_ipc: только ovr_core + ovr_state)
  - tests/unit/test_snapshot_stream.cpp — (публикация/потребление/пропуск/повреждение слота)
  - tests/bench/bench_frame.cpp — (меряет этапы пайплайна по §5 и печатает таблицу)
  - tests/bench/CMakeLists.txt — (таргет ovr_bench_frame; в ctest только smoke-прогон)
Контракт (из ARCHITECTURE.md:538):
  namespace ovr::ipc
    struct SnapshotHeader {
      std::uint32_t magic;              // 'OVRS'
      std::uint16_t schema_version;
      std::uint16_t header_size;
      std::uint64_t seq;
      std::uint64_t tick_client;
      std::uint64_t tick_server;
      std::uint32_t payload_size;
      std::uint32_t checksum;           // crc32 по payload
    };
    class SnapshotStream {
     public:
      virtual bool publish(const state::HostState&) = 0;
      virtual bool try_consume(state::HostState& out) = 0;
    };
    std::unique_ptr<SnapshotStream> create_shm_ring(std::string_view name);
  Семантика: ring ровно из 4 слотов, SPSC, без мьютексов (атомарные head/tail, acquire/release).
  Число слотов — `kSnapshotSlots` (§10.15, ARCHITECTURE.md:955), объявлена один раз в заголовке
  этой задачи; «4» в коде и в тестах — только через неё.
  publish() никогда не блокируется: если потребитель отстал, старый слот просто затирается,
  seq монотонно растёт. try_consume(): если нового слота нет => false и out не тронут; если
  checksum не сходится или magic/schema_version не те => false и потребитель рисует прошлый
  снапшот (сам решает, здесь только false). Имя shm — input-параметр как есть, без префиксов;
  деструктор делает shm_unlink. По умолчанию весь ipc выключен (7.1), main.cpp его не добавляет.
Крайние случаи: E26, E27, E35 (каталог ARCHITECTURE.md:999 §11).
Ограничения:
  Никаких сокетов, именованных pipes, второго процесса в тестах. Никаких аллокаций в publish()/
  try_consume() (payload сериализируется в слот на месте). Бенчмарк не имеет права требовать
  хост: данные берутся из FakeBackend (T005) и фикстур `tests/golden/**`, если они уже есть,
  иначе — из локально собранных подставных кадров. Бенчмарк не спит больше 200 мс суммарно.
DoD / тест:
  1) publish/try_consume 1000 раз подряд → seq монотонны, все checksum валидны, потерь нет;
  2) потребитель спит «руками» (не вызывать try_consume) 6 публикаций подряд → следующий consume
     отдаёт САМЫЙ НОВЫЙ снапшот, а publish() ни разу не вернул false;
  3) E26: портить 1 байт payload в слоте → try_consume() возвращает false, out не изменён;
  4) E27: портить magic/schema_version → false; два reader'а на одном имени не создаются (shm_open
     с новым именем в тесте не конфликтует);
  5) no-alloc: publish/try_consume под счётным аллокатором из T005 → 0 аллокаций после прогрева;
  6) `./build/tests/bench/ovr_bench_frame` печатает таблицу и финальная строка вида
     `total=<x> ms budget=1.8 ms ok|FAIL`; числовые пороги из §5: read<=0.600, parse<=0.400,
     features<=0.300, draw<=0.500, total<=1.800 мс; при FAIL задача НЕ принимается.
     На моменте T015 существуют только read и parse (фичи — T016/T031, отрисовка — T012/T014),
     поэтому их строки таблицы печатаются как `not-measured`, а `total` считается по измеренным
     этапам и помечается `partial=features,draw`. Пороги read/parse обязательны; `not-measured`
     выдуманным нулём не считается (P10); полный прогон таблицы — DoD T033.
  7) smoke-прогон бенча внутри ctest (одна итерация, без порогов) зелёный;
  8) `ctest --test-dir build --output-on-failure` — весь набор зелёный.
  9) 400 агентов в фикстуре (E35): publish() укладывается в бюджет §5 и не делает аллокаций;
     снапшот влезает в слот (иначе в DoD-отчёте «ёмкость слота недостаточна», а не молчаливое
     усечение payload).
Не трогать: src/main.cpp (кроме case'а, если §5 требует — иначе не трогать вовсе), src/render/**,
src/overlay/**, include/ovr/state/host_state.hpp (это T007).
```

---

# M3 — Состояние хоста: структура, парсер, ресурс

Здесь «сырые байты» превращаются в `HostState`, с которым дальше работают фичи. Ключевая
идея: парсер ничего не знает про отрисовку и умеет честно говорить «не знаю» через
`validity_bits` (`ARCHITECTURE.md:211`). Поле без подтверждённого смещения = sentinel,
а не угаданное число (`RESEARCH.md:100`).

### T007 — HostState/Agent: публичные типы состояния

```text
ID: T007
Milestone: M3. Зависимости: T001, T006, T021.
Цель:
  Дать всему проекту единственный публичный тип «что сейчас в хосте» — HostState,
  ActorSnapshot, Agent — чтобы фичи и отрисовка не трогали память.
Вход:
  - ARCHITECTURE.md:211 (§3.5) — ActorSnapshot и HostState заданы дословно, трогать нельзя.
  - ARCHITECTURE.md:696 (§10.2) — FrameArena: заголовок тоже этой задачи, цитата ниже.
  - ARCHITECTURE.md:716 (§10.3) — Agent: заголовок тоже этой задачи, цитата ниже.
  - ARCHITECTURE.md:731 (§10.4) — раскладка FieldBit: заголовок тоже этой задачи, цитата ниже.
  - ARCHITECTURE.md:955 (§10.15) — `kMaxActors`, `kMaxAgents` объявляются здесь.
  - ARCHITECTURE.md:114 (§3.1), ARCHITECTURE.md:154 (§3.2) — чем парсер читает память.
  - ARCHITECTURE.md:175 (§3.3) — откуда берутся смещения, `is_stale()`.
  - ARCHITECTURE.md:41 (§2) — state/ не имеет права знать про render/, overlay/, windows.h.
  - Заголовки T001 (`include/ovr/core/`), T006 (`include/ovr/memory/typed_reader.hpp`) — читать как данность.
  - RESEARCH.md:127 (§5) — что реально подтверждено в билде 6985, чего нет.
Выход:
  - include/ovr/state/host_state.hpp (ActorSnapshot и HostState — буква в букву из §3.5; Agent — ниже).
  - include/ovr/state/validity.hpp (enum `FieldBit`, `field_ok`) — раскладка зафиксирована
    в ARCHITECTURE.md:731 (§10.4), цитата ниже.
  - include/ovr/state/frame_arena.hpp (FrameArena + `kMaxActors`, `kMaxAgents`) — контракт
    зафиксирован в ARCHITECTURE.md:696 (§10.2).
  - src/state/host_state.cpp (реализация `to_string`/дебажного описания Agent и ActorSnapshot, без IO).
  - tests/unit/test_state.cpp ( POD-семантика, копирование, default-значения, size/align стабильны).
Контракт (из ARCHITECTURE.md:211):
  struct ActorSnapshot { core::Vec3 pos; int team; int health, max_health, mana, max_mana;
                        int level; std::uint32_t flags; bool alive, visible_to_us; };
  struct HostState { int tick_client, tick_server; double current_time; int map_width, map_height;
                     core::Vec3 camera_origin; core::Mat4x4 view_matrix; float fov_deg, aspect,
                     znear, zfar; std::span<ActorSnapshot> actors; std::span<Agent> agents; // view в арену
                     std::uint64_t validity_bits; bool is_background_map; int sign_on_state; };
  Поля ActorSnapshot/HostState и их порядок фиксированы §3.5; добавлять поля в ActorSnapshot нельзя.
  `Agent` в §3.5 назван, но не определён — контракт зафиксирован в ARCHITECTURE.md:716 (§10.3):
    struct Agent { std::uint32_t raw_index; core::Vec3 pos; int team;
                  int health, max_health; int gold, last_hits, xp;
                  std::uint32_t flags; bool alive, is_actor, is_minion, is_guard,
                            is_building, is_beacon; std::uint16_t entity_type; };
  `entity_type` — числовой класс из дампа T011 (`RESEARCH.md:47`), переводить в bool-флаги
  только по таблице из `contracts/fields.client-6985.json`; своих догадок о числовом классе нет.
  Sentinel-семантика: любое int-поле, смещение которого отсутствует в
  `contracts/fields.client-6985.json`, равно -1; любой bool в таком случае false;
  `gold/last_hits/xp` помечены как not-established до появления дампа (`RESEARCH.md:100`).
  `validity.hpp` — дословно из §10.4 (ARCHITECTURE.md:731): `enum class FieldBit : std::uint8_t`
  с участками 0..15 кадр / 16..39 актор / 40..63 агент и `bool field_ok(std::uint64_t, FieldBit)`;
  номера бит менять нельзя, `*_reserved*` остаются свободными (§13.3).
  `frame_arena.hpp` — дословно из §10.2 (ARCHITECTURE.md:696): `begin_frame()`, `actors()`,
  `agents()`, `used_actors()`, `used_agents()`, `overflowed()`; arena аллоцируется один раз и
  переживает кадры, копировать её нельзя. `kMaxActors == 10`, `kMaxAgents == 64` (§10.15) —
  здесь, а не в парсере; ёмкость agents() учитывает ещё `max_clients` из реестра (данные).
Крайние случаи: E9, E44 (каталог ARCHITECTURE.md:999 §11).
Ограничения:
  - Только заголовки `core/`, `memory/`, `offsets/` в include'ах; `render/`, `overlay/`, `windows.h` — запрет (§2:41).
  - Никаких аллокаций внутри самих типов: actors/agents — view-ы в FrameArena, а не владельцы (P4).
  - Никаких `static`--singleton'ов и глобального состояния в `src/state/**`.
DoD / тест:
  1) `grep -n "struct ActorSnapshot" -A 3 include/ovr/state/host_state.hpp` совпадает с §3.5 посимвольно
     (тот же набор и порядок полей); то же для HostState.
  2) `grep -RE "render/|overlay/|windows\.h|dxgi" include/ovr/state src/state` — пусто.
  3) test_state.cpp собирает HostState значением, копирует и сравнивает все поля;
     проверяет, что default-конструированный объект даёт validity_bits == 0 и пустые span-ы.
  4) static_assert на отсутствие виртуальных деструкторов/наследования в ActorSnapshot и Agent.
  5) E9: FrameArena: после begin_frame() счётчики нули; вставка сверх kMaxActors даёт
     `overflowed() == true` и не пишет лишнего; 100 циклов begin_frame() дают 0 аллокаций
     после первого (счётный аллокатор); арена не знает ни про память, ни про отрисовку
     (`grep -RE "memory/|render/" include/ovr/state/frame_arena.hpp` — пусто).
  6) номера FieldBit сверяются с §10.4 построчно (diff цитаты и заголовка пустой).
  7) E44: фикстура с `current_time` меньше предыдущего кадра -> `HostState` передаёт значение
     как есть: в типах состояния нет часов, дельт и деления на дельту (считает фича, T018/T028).
  8) `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j &&
     ctest --test-dir build --output-on-failure --tests-regex state` — зелёные, без хоста.
Не трогать: contracts/** (кроме чтения), src/state/parse_frame/** (это T008), src/main.cpp.
```

### T008 — parse_frame: обход entity list и сетевых слотов

```text
ID: T008
Milestone: M3. Зависимости: T002, T003, T006, T007, T011.
Цель:
  Реализовать единственную точку «память -> HostState»: один вызов за кадр, ноль аллокаций,
  честная отметка недостоверных полей через validity_bits.
Вход:
  - ARCHITECTURE.md:211 (§3.5) — `HostState parse_frame(const memory::RawFrame&, const
    offsets::OffsetRegistry&, state::FrameArena&);` — сигнатура с ареной, P4.
  - ARCHITECTURE.md:194 (§3.4) — TypedReader, `read_or_false`; ARCHITECTURE.md:175 (§3.3) — OffsetRegistry.
  - ARCHITECTURE.md:679 (§10.1) — RawFrame: что вернула пачка и почему `complete == false` не запрет разбора.
  - ARCHITECTURE.md:936 (§10.14) — FrameSource: заголовок тоже этой задачи, цитата ниже.
  - ARCHITECTURE.md:955 (§10.15) — `kEntityScanLimit`, `kNullSlotRunLimit`, `kNetSlotStride` объявляются здесь.
  - RESEARCH.md:127 (§5) — структура обхода: stride сетевого слота 24 байта, `entityCount` != highest index,
    `dwNetworkGameClient_maxClients` = 576, `dwNetworkGameClient_deltaTick` == `serverTickCount` == 588.
  - RESEARCH.md:47 (§2.1) — цепочка GameEntitySystem -> entity list; RESEARCH.md:183 (§7) — view-цепочка.
  - contracts/fields.client-6985.json (артефакт T011) — единственный источник смещений внутри сущностей.
  - include/ovr/state/host_state.hpp (T007) — читать как данность.
Выход:
  - src/state/parse_frame/entity_list.cpp (обход списка сущностей: GameEntitySystem -> entity list,
    шаг 24 байта на сетевой слот, остановка после `kNullSlotRunLimit` подряд пустых слотов,
    верхняя граница `kEntityScanLimit`).
  - src/state/parse_frame/network.cpp (тики `clientTickCount`/`serverTickCount`, `signOnState`,
    `isBackgroundMap`, размеры окна).
  - src/state/parse_frame/view_chain.cpp (camera origin, view matrix, fov/aspect/near/far
    по цепочке `client.dll!dwViewRender` из contracts/fields.client-6985.json).
  - src/state/parse_frame/parse_frame.cpp (сборка HostState, установка validity_bits).
  - include/ovr/state/parse_frame/parse_frame.hpp (объявление одной функции из §3.5).
  - include/ovr/state/frame_source.hpp (FrameSource — дословно §10.14) и
    src/state/frame_source/frame_source.cpp (план чтения: какие символы, какие размеры, как
    разложить ответ пачки в RawFrame::bytes). Здесь же объявляется право T008 на этот файл:
    соседним задачам он только читается.
  - tests/unit/test_parse_frame.cpp (парсинг на фейковом RawFrame: подставные байты -> ожидаемый HostState).
  - tests/unit/test_frame_source.cpp (один read_batch() за кадр, поведение при is_stale()).
Контракт (из ARCHITECTURE.md:211 и ARCHITECTURE.md:936):
  HostState parse_frame(const memory::RawFrame&, const offsets::OffsetRegistry&, state::FrameArena&);
  Ровно один вызов за кадр. Возврат по значению фиксирован §3.5, поэтому внутри парсера держится
  арена `state::FrameArena` (объявляет T007) живёт между кадрами: parse_frame пишет в готовые
  слоты и возвращает span-ы; после первого кадра аллокаций ноль (PRINCIPLES P4).
  `offsets.is_stale() == true` => возвращать HostState с `validity_bits == 0` и пустыми span-ами,
  ничего не выдумывая. Поле считается достоверным только если его смещение есть в реестре И
  чтение удалось; иначе соответствующий бит `FieldBit` (T007) снят.
  FrameSource — дословно из §10.14 (ARCHITECTURE.md:936): конструктор от backend/registry/arena,
  `bool read_frame()` (ровно один read_batch() за кадр), `const memory::RawFrame& raw() const`,
  `std::uint32_t requests() const`. План чтения живёт здесь; `parse_frame` о плане ничего не
  знает и остаётся чистой функцией (§12.1 Controller). `is_stale() == true` => `read_frame()`
  возвращает false и к памяти не обращается вовсе (P9).
  Порядок один за кадр: `read_frame()` -> `parse_frame(raw(), registry, arena)`; никто другой
  к памяти за кадр не обращается (ни `parse_frame`, ни фичи).
Крайние случаи: E4, E5, E6, E8, E9, E10, E48 (каталог ARCHITECTURE.md:999 §11).
Ограничения:
  - Никаких литералов смещений в .cpp: только имена из OffsetRegistry (`RESEARCH.md:100`).
  - `entityCount` использовать как нижнюю оценку, а не как границу обхода; границу задавать
    именованной константой и ограничивать сверху, чтобы не читать за карту.
  - Никаких аллокаций, итераторов-владений указателями хоста, sleep, /proc, интернета в парсере.
  - Один процесс, один поток (§5:416); потокобезопасность не требуется, гонки не добавлять.
DoD / тест:
  1) test_parse_frame.cpp строит RawFrame из `std::array<std::uint8_t, N>` с вручную расставленными
     значениями и проверяет: 2 актора + 3 пажа -> `actors.size()==2`, `agents.size()`>=3, координаты совпали.
  2) E8: тест «empty slot run»: слоты 0..3 заполнены, 4..20 пустые -> парсер останавливается и не читает дальше лимита.
  3) E4: тест «stale registry» — реестр с другим `ClientVersion` -> `validity_bits == 0`, actors и agents пусты.
  4) Тест «missing field»: смещения `m_iHealth` убрано из фикстуры -> бит снят, `health == -1`.
  5) Тест на аллокации: счётный аллокатор вокруг 100 вызовов parse_frame даёт 0 аллокаций после первого кадра.
  6) E5: адрес в цепочке == 0 на втором шаге -> ok==false, значение обнулено, бит достоверности снят, обход кадра продолжается;
  7) `grep -RE "[^0-9][0-9]{6,}[^0-9]" src/state/parse_frame` — пусто (нет хардкода смещений).
  8) E48: FrameSource на FakeBackend со счётчиком: `requests() <= kMaxReadRequests` и
     `batch_calls == 1` на кадр (ровно один read_batch()); при `is_stale()` — `read_frame() == false`
     и ноль обращений к памяти; `raw().complete == false` после обрезанной пачки, парсинг при
     этом продолжается и снимает биты (§10.1, E6).
  9) пустой `RawFrame` (первый кадр / `bytes.empty()`) => `validity_bits == 0`, span-ы пусты,
     падений нет (E10); переполнение арены в тесте => `overflowed() == true` и разбора дальше нет (E9).
  10) `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j &&
     ctest --test-dir build --output-on-failure --tests-regex parse_frame` — зелёные без хоста.
Не трогать: contracts/**, src/render/**, include/ovr/state/host_state.hpp (только чтение).
```

### T009 — Golden-снапшоты: фикстуры и по-полевая сверка

```text
ID: T009
Milestone: M3. Зависимости: T006, T007, T008, T011.
Цель:
  Зафиксировать поведение парсера эталонными дампами, чтобы любая правка смещений или парсера
  падала в тесте, а не в хосте.
Вход:
  - ARCHITECTURE.md:211 (§3.5), ARCHITECTURE.md:154 (§3.2) — что именно сериализуем в эталон.
  - ARCHITECTURE.md:925 (§10.13) — формат эталонной фикстуры: имена полей и роль `null`.
  - ARCHITECTURE.md:679 (§10.1) — что лежит в `raw/*.bin` (сырые байты `RawFrame::bytes`).
  - ARCHITECTURE.md:731 (§10.4) — какой бит считается снятым для поля со `null`.
  - ARCHITECTURE.md:628 (§8.2) — требования приёмки; ARCHITECTURE.md:456 (§5) — бюджеты.
  - RESEARCH.md:100 (§4, методология) — правило «поле принимается по >=2 источникам или своему дампу».
  - RESEARCH.md:218 (§8) — как снимать дампы и почему нельзя просто запомнить адреса.
  - src/state/parse_frame/** (T008), contracts/evidence/client-6985/ (T011) — читать как данность.
Выход:
  - tests/golden/frame-6985-menu.json (дамп одного кадра в меню: тики, sign_on_state, размеры окна) —
    (снимается один раз живьём и коммитится; потом только читается).
  - tests/golden/frame-6985-play.json (дамп кадра в стандартном сетевом режиме: 10 акторов + часть пажей).
  - tests/golden/raw/frame-6985-menu.bin, tests/golden/raw/frame-6985-play.bin (сырые байты RawFrame,
    чтобы тест не зависел от живой памяти).
  - tests/golden/README.md (руководство P8/Q9: как одной командой на Windows снять дампы живьём,
    закоммитить их с MANIFEST.sha256, переснять после патча и что делать при провале теста).
  - tests/golden/MANIFEST.sha256 — (sha256 на каждый .bin и .json; приёмка сверяет хэш, RESEARCH.md:115)
  - tests/unit/test_golden_state.cpp (читает .bin, зовёт parse_frame, сверяет HostState по полям с .json).
Контракт (из ARCHITECTURE.md:211):
  Парсер из §3.5 вызывается ровно один раз на каждый golden-дамп; сравнение полей — только через
  публичные поля HostState/ActorSnapshot/Agent из T007. JSON-эталон повторяет имена полей §3.5 1-в-1;
  для недостоверных полей в эталоне стоит `null`, и тест требует `validity_bits` со снятым битом.
  Контракт формата фикстуры зафиксирован в ARCHITECTURE.md:925 (§10.13):
  { "client_version": <int>, "raw": "<имя .bin>", "state": { "<поле из §3.5 или §10.3>": <значение|null>, ... } }
  Имена полей — ровно имена из §3.5 и §10.3, ни одного нового имени; `null` => у соответствующего
  бита `FieldBit` (§10.4) снят флаг; `.bin` — сырые байты без заголовков (§10.1).
Крайние случаи: E3 (каталог ARCHITECTURE.md:999 §11).
Ограничения:
  - Тестам нельзя читать /proc, память хоста, GPU, интернет и спать (§8.2:588); только файлы из tests/golden/.
  - .bin не генерировать во время теста; генерация — отдельный скрипт, вызываемый вручную.
  - Не хранить в эталоне поля, которые T008 помечает not-established (кроме как `null`).
DoD / тест:
  1) `ctest --test-dir build --output-on-failure --tests-regex golden` проваливается, если вручную
     изменить один байт координаты в tests/golden/raw/frame-6985-play.bin (проверить локально и вернуть как было).
  2) Тест печатает diff вида `field=m_pos expected=... actual=...` при несовпадении — постранично, а не весь объект.
  3) `python3 -c "import json;json.load(open('tests/golden/frame-6985-play.json'))"` без ошибок; оба .json проходят ту же проверку.
  4) `grep -c "\"client_version\": 6985" tests/golden/*.json` — по 1 в каждом файле.
  5) E3: каталога `tests/golden/` нет -> тест печатает not-established и список фикстур, которые надо добыть, чисел-заглушек не выводит (P10);
  6) Тесты проходят при `ctest --test-dir build --output-on-failure` целиком, без запущенного хоста.
Не трогать: src/state/parse_frame/** (только чтение), contracts/**, src/main.cpp.
```

---

# M4 — Отрисовка: кадры, проекция, оверлей

Пиксели отделены от состояния: фичи складывают примитивы в `FrameDraw`, проекция
переводит мир в экран, оверлей рисует. Окно ровно одно, без подсадки модулей и без перехватов —
это условие выживания после патчей (`RESEARCH.md:259`, риск R1 в `ARCHITECTURE.md:501`).

### T012 — FrameDraw: буфер примитивов без аллокаций

```text
ID: T012
Milestone: M4. Зависимости: T001, T015.
Цель:
  Дать фичам единственный способ «хочу что-то нарисовать»: очередь примитивов с z-order'ом,
  которая за кадр выделяет память ровно один раз.
Вход:
  - ARCHITECTURE.md:235 (§3.6) — Line/Rect/Circle/Poly/Text, DrawItem, фабрики, clear/items/reserve.
  - ARCHITECTURE.md:264 (§3.7) — кто позже превращает мировые координаты в экранные.
  - ARCHITECTURE.md:363 (§3.11) — как фича получает FrameDraw в `run()`.
  - ARCHITECTURE.md:456 (§5) — бюджет draw <= 0.5 мс и no-alloc в горячем пути.
  - ARCHITECTURE.md:955 (§10.15) — `kDrawReserve` объявляется здесь; значение из замера T015,
    до замера — дефолт таблицы, менять его может только ревьюер по бенчмарку.
  - ARCHITECTURE.md:41 (§2) — render/ не знает про overlay-бэкенды и windows.h.
Выход:
  - include/ovr/render/frame_draw.hpp (типы и FrameDraw из §3.6 дословно).
  - src/render/frame_draw.cpp (фабрики, clear(), items(), reserve(), порядок z-order).
  - tests/unit/test_render.cpp (семантика примитивов + счётный аллокатор).
Контракт (из ARCHITECTURE.md:235):
  Line{Vec2 a,b; rgba; thickness}, Rect{Vec2 min,max; rgba; thickness; bool fill},
  Circle{Vec2 center; float radius; rgba; thickness; bool fill},
  Poly{std::span<const Vec2> pts; rgba; thickness; bool fill},
  Text{Vec2 pos; rgba; float size; std::string_view utf8};
  using DrawItem = std::variant<...>;
  фабрики line(a,b,rgba,thickness=1.f), rect(mn,mx,rgba,1.f,false), circle(c,r,rgba,1.f,false),
  poly(pts,rgba,1.f,false), text(pos,utf8,rgba,size=13.f), clear(), items() (=z-order), reserve(n).
  `reserve(n)` — единственный аллок за кадр; `items()` возвращает элементы в порядке вставки;
  `Poly::pts` — заимствованный `std::span`, владельца буфера у FrameDraw нет (вызывающий держит
 .points живыми до конца кадра — это фиксируется комментарием в заголовке).
  `constexpr std::uint32_t kDrawReserve` (§10.15, ARCHITECTURE.md:955) — ёмкость прогрева на кадр;
  объявлена ровно один раз в этом заголовке, в comment — ссылка на замер T015. Пул растёт только
  внутрь reserve(), никогда — внутри clear()/items().
Крайние случаи: E37, E38 (каталог ARCHITECTURE.md:999 §11).
Ограничения:
  - Никаких вызовов DX/Vulkan/ImGui в src/render/**: это чистый CPU-буфер команд.
  - `std::string_view` в Text обязан указывать на строку с временем жизни дольше кадра
    (тест на literal/static-буфер обязателен).
  - Никаких аллокаций в clear() и items(); reserve(0) не должен аллоцировать.
DoD / тест:
  1) Тест со счётным аллокатором: reserve(N), затем N вызовов фабрик -> 0 аллокаций.
  2) Тест «reserve(0) в горячем пути» (E38): после прогрева reserve(kDrawReserve) и clear()
     ещё 100 кадров дают 0 аллокаций и `items().size()==0`; на холодном старте reserve(0) +
     3 примитива -> ровно 1 аллок (рост пула), а не 3.
  3) Тест z-order: items()[i] совпадает с i-м вставленным примитивом по типу и координатам.
  4) Тест: после clear() items().size()==0 и новых аллокаций нет.
  5) E37: `Text::utf8` указывает на static/literal строку вызывающего -> тест проверяет, что FrameDraw её не копирует и не владеет ею (время жизни — вызывающий, §3.6);
  6) `grep -RE "d3d|vulkan|imgui|windows\.h" src/render include/ovr/render` — пусто.
  7) `grep -c kDrawReserve include/ovr/render/frame_draw.hpp` == 1 и `grep -rn "kDrawReserve =" src include`
     даёт ровно это объявление (константа в одном месте, §10.15).
  8) `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j &&
     ctest --test-dir build --output-on-failure --tests-regex render` — зелёные.
Не трогать: include/ovr/render/world_to_screen.hpp (это T013), src/overlay/** (это T014).
```

### T013 — world_to_screen / screen_to_world_ray

```text
ID: T013
Milestone: M4. Зависимости: T001, T011, T012.
Цель:
  Закрыть единственную математическую зависимость меток: перевод мировых координат в пиксели
  клиентского окна и обратно, только по данным из HostState.
Вход:
  - ARCHITECTURE.md:264 (§3.7) — View, world_to_screen, screen_to_world_ray, семантика координат.
  - ARCHITECTURE.md:211 (§3.5) — откуда View берётся (view_matrix, camera_origin, fov, aspect, near/far).
  - RESEARCH.md:183 (§7) — почему живой матрицы в 6985 пока нет и почему тесты только синтетические.
  - ARCHITECTURE.md:501 (§6, риск R4) — T011 в критическом пути перед T013.
  - src/render/frame_draw.hpp (T012) — читать как данность.
Выход:
  - include/ovr/render/world_to_screen.hpp (View и две функции из §3.7 дословно).
  - src/render/world_to_screen.cpp (реализация на Mat4x4 из core/).
  - tests/unit/test_world_to_screen.cpp (исключительно синтетические матрицы).
Контракт (из ARCHITECTURE.md:264):
  struct View { core::Mat4x4 matrix; core::Vec3 cam_origin; float fov_deg{}, aspect{}, znear{}, zfar{}; bool valid{}; };
  bool world_to_screen(const View&, Vec3 world, Vec2& out_screen);
  bool screen_to_world_ray(const View&, Vec2 screen, Vec3& out_dir);
  Матрица row-major; выходные координаты — пиксели клиентского окна, начало слева сверху;
  false возвращается при `!valid`, при `w <= 0` (точка за камерой) и при выходе за границы окна;
  `screen_to_world_ray` возвращает нормализованный вектор направления, начало луча — `cam_origin`.
  `out_screen` при false не трогать (неопределённое значение запрещено использовать).
Крайние случаи: E11, E12, E13, E47 (каталог ARCHITECTURE.md:999 §11).
Ограничения:
  - Никаких DX/Vulkan/ImGui заголовков; никаких обращений к памяти хоста.
  - Никаких магических констант проекции: всё из View; aspect<=0 или fov_deg<=0 => false.
  - Реальных матриц из хоста в тесты не подставлять (их ещё нет, RESEARCH.md:183).
DoD / тест:
  1) Тест «тождественная матрица + камера в начале координат»: точка (0,0,-1000) даёт экранный центр окна.
  2) E11: тест «за камерой»: точка (0,0,+1000) => false, out_screen не изменён (проверяется стражем-значением).
  3) Тест round-trip: world_to_screen -> screen_to_world_ray даёт направление, colinearное вектору в точку
     (скалярное сравнение с допуском 1e-4f).
  4) E12: тест вне окна: x за шириной => false; `valid == false` => false для обеих функций.
  5) Тест симметрии: точка справа от центра даёт x > центра, зеркальная — x < центра.
  6) E13: `fov_deg <= 0`, `aspect <= 0`, `znear == zfar` -> false; ни NaN, ни inf, ни деления на ноль.
  7) `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j &&
     ctest --test-dir build --output-on-failure --tests-regex world_to_screen` — зелёные.
  8) E47: радиус 0 и отрицательный радиус -> 0 сегментов либо минимум по константе; камера
       ровно в центре кольца -> масштабируемый радиус не даёт inf/NaN, проекция возвращает true|false
       без деления на ноль.

Не трогать: src/state/parse_frame/view_chain.cpp, src/render/frame_draw.cpp, src/overlay/**.
```

### T014 — Оверлей: одно окно DX11 + ImGui

```text
ID: T014
Milestone: M4. Зависимости: T001, T012, T013.
Цель:
  Вывести FrameDraw на экран поверх хоста: одно topmost-прозрачное окно без рамки, vsync-синхронное,
  без перехвата фокуса и без подсадки модуля в процесс хоста.
Вход:
  - ARCHITECTURE.md:289 (§3.8) — OverlayDesc, IOverlay, create_directx11/create_vulkan.
  - ARCHITECTURE.md:235 (§3.6) — что рисует present(); ARCHITECTURE.md:264 (§3.7) — координаты уже экранные.
  - RESEARCH.md:259 (§9) — требования к окну: без tool-window-мигания, без смены фокуса, GDI запрещено.
  - RESEARCH.md:283 (§10) — какой графический бэкенд выбран как основной и почему Vulkan — второй.
  - ARCHITECTURE.md:501 (§6, риск R1) — никаких перехватов и подсадки модулей.
  - ARCHITECTURE.md:456 (§5) — present <= 0.5 мс CPU на кадр, не блокировать дольше одного кадра.
Выход:
  - include/ovr/overlay/overlay.hpp (OverlayDesc, IOverlay, обе фабрики из §3.8).
  - src/overlay/dx11/dx11_overlay.cpp (реализация IOverlay на Direct3D 11 — единственное окно).
  - src/overlay/vulkan/vulkan_overlay.cpp (основа IOverlay, собираемый, но не включаемый в рантайм;
    помечен как not-established по RESEARCH.md:283).
  - src/overlay/command_buffer.cpp (FrameDraw -> список вершин/текстура глифов, чистая CPU-логика).
  - src/overlay/window.cpp (создание окна: topmost, layered/transparent, без рамки, без tool-window,
    клик-_through по умолчанию; ввод события мыши только для чтения курсора).
  - third_party/imgui/ (vendored ImGui, статически; third_party/README.md с точной версией и ссылкой).
  - tests/unit/test_overlay_command_buffer.cpp (FrameDraw -> ожидаемый список вершин, без окна и GPU).
Контракт (из ARCHITECTURE.md:289):
  struct OverlayDesc { uint32_t width, height; bool topmost, transparent; std::string_view title; };
  class IOverlay { virtual bool init(const OverlayDesc&)=0;
                   virtual void resize(std::uint32_t w, std::uint32_t h)=0;
                   virtual bool present(const render::FrameDraw&)=0;
                   virtual bool next_event(core::Vec2& cursor, bool& pressed)=0; };
  IOverlay* create_directx11(); IOverlay* create_vulkan();
  Одно окно на весь процесс; `init()` вызывается один раз, `resize()` — только при смене размера клиента;
  `present()` синхронизирован с vsync и не должен блокировать дольше одного кадра;
  `next_event()` возвращает false, когда событий нет (не спать, не крутить поток).
  Меню по хоткею работает, даже когда окно не в фокусе; фокус окну не передавать никогда.
Крайние случаи: E28 (каталог ARCHITECTURE.md:999 §11).
Ограничения:
  - GDI/GDI+ не использовать (RESEARCH.md:259); DirectX 9 не использовать.
  - Никаких перехватов ввода, SetWindowsHookEx, GetAsyncKeyState-поллинга в цикле без сна (опросить через IInputBackend позже, T019).
  - ctest не должен создавать окно и трогать GPU: вся проверяемая логика в command_buffer.cpp.
  - third_party/** не править вручную; обновление — только заменой каталога (записать в third_party/README.md).
DoD / тест:
  1) test_overlay_command_buffer.cpp: 1 rect + 1 text + 1 circle -> ожидаемое число треугольников/вершин
     и ожидаемый порядок отрисовки (z-order из items()).
  2) Тест: пустой FrameDraw -> present не вызывался ни разу в тесте, command_buffer даёт 0 вершин и 0 аллокаций.
  3) `grep -RE "SetWindowsHookEx|CreateThread|Gdi|BitBlt" src/overlay` — пусто.
  4) `grep -n "WS_EX_TOOLWINDOW\|WS_EX_LAYERED\|WS_EX_TOPMOST" src/overlay/window.cpp` — все три присутствуют;
     `WS_EX_APPWINDOW` отсутствует.
  5) Сборка с `-DOVR_OVERLAY=DX11` и с `-DOVR_OVERLAY=VULKAN` проходит; без Флага — DX11 по умолчанию.
  6) Вручную (записать вывод в tests/golden/README.md, не в ctest): в меню хоста окно поверх, не мигает,
     фокус не забирает, закрытие процесса наблюдателя не оставляет «чёрного квадрата».
  7) E28: `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j &&
     ctest --test-dir build --output-on-failure` — зелёные на Linux без окна (overlay-секция пропускается через метку).
Не трогать: src/render/**, include/ovr/features/** (это T016), src/input/** (это T019).
```

### T016 — `FeatureRegistry`: реестр фич и правило «одной строки»

```text
ID: T016
Milestone: M4. Зависимости: T007, T012, T013, T014, T021.
Цель:
  Каркас реестра из §3.11: единственная точка, где фичи регистрируются и вызываются, чтобы
  любая следующая фича была «одна директория + один класс + одна строка регистрации». Первая
  настоящая фича (метки акторов) — это T031; здесь ни одной гипотезы о хосте нет.

Вход:
  - ARCHITECTURE.md:363 (§3.11) — IFeature, FeatureRegistry, список запретов для фич.
  - ARCHITECTURE.md:235 (§3.6) и ARCHITECTURE.md:264 (§3.7) — типами каких примитивов оперирует run().
  - ARCHITECTURE.md:336 (§3.10) — Config как единственный источник настроек (ключ `<имя>.enabled`).
  - ARCHITECTURE.md:211 (§3.5) — HostState как единственный источник данных.
  - ARCHITECTURE.md:444 (§4.2) — рёбра: src/features/** не включает memory/**.
  - ARCHITECTURE.md:999 (§11) — строки E32, E33, E34 (они же обязательные тесты ниже).

Выход:
  - include/ovr/features/feature.hpp (IFeature из §3.11 дословно).
  - include/ovr/features/feature_registry.hpp + src/features/feature_registry.cpp
    (FeatureRegistry::add/run_all/features; вызов фич в алфавитном порядке по name()).
  - src/features/CMakeLists.txt (цель ovr_features: ovr_core + ovr_state + ovr_render + ovr_config).
  - tests/unit/test_feature_registry.cpp (реестр на двух тестовых фичах-заглушках).

Контракт (из ARCHITECTURE.md:363):
  class IFeature { virtual std::string_view name() const=0;
                   virtual void run(const state::HostState&, const config::Config&, render::FrameDraw&)=0; };
  class FeatureRegistry { void add(std::unique_ptr<IFeature>);
                          void run_all(const state::HostState&, const config::Config&, render::FrameDraw&);
                          std::span<IFeature* const> features() const; };
  Фича не читает память, не хранит указатели хоста между кадрами, не аллоцирует в run(),
  не меняет convar-ы. `run_all` вызывает каждую фичу ровно один раз за кадр и не бросает наружу.
  Ключ `<имя_фичи>.enabled` читает сама фича, а не реестр: реестр не знает имён фич (P6, §12.3).
  Порядок вызова — алфавит по name(): он не зависит от порядка merge'а веток (P7).

Крайние случаи: E16, E32, E33, E34 (каталог ARCHITECTURE.md:999 §11).
Ограничения:
  - В `src/features/feature_registry.cpp` не должно быть ни одного имени конкретной фичи.
  - Порядок вызова фиксируется при add(); сортировка и аллокации внутри run() запрещены (P4).
  - Никаких потоков и процессов: один поток, один процесс (ARCHITECTURE.md:456 §5).
DoD / тест:
  1) 2 зарегистрированные заглушки: run_all вызывает обе ровно один раз (счётчики в заглушках),
     `features().size() == 2`, порядок вызовов — алфавитный по name().
  2) E32: вторая заглушка с тем же `name()` — `add()` её игнорирует, печатает ровно одну строку
     в stderr, `features().size()` не меняется (иначе один ключ конфига читали бы две фичи).
  3) E33: заглушка бросает исключение в run() -> run_all не выпускает наружу, остальные фичи
     вызваны полностью, примитивы этого кадра не «плывут».
  4) E34: пустой HostState (0 акторов, 0 агентов) -> заглушка вызвана, 0 примитивов, 0 аллокаций.
  5) счётный аллокатор: после прогрева run_all даёт 0 аллокаций (P4).
  6) diff цитаты §3.11 и `include/ovr/features/feature.hpp` пустой (подпись дословно, §13.3).
  7) `grep -rn '"vision"\|"last_hit"\|"timers"' src/features/feature_registry.cpp` -> пусто.
  8) общее правило 14 (до T023 рёбра include проверяются вручную по §4.2): src/features/**
     не включает memory/**, include/ovr/features/** не включает render/**-бэкенды.
  9) E16: `markers_enabled == false` -> `run_all` не вызывает ни одной заглушки: 0 примитивов, счётчики вызовов нули.
  10) `cmake --build build && ctest --test-dir build --output-on-failure` — зелёные без хоста.
Не трогать: src/main.cpp (первую строку `registry.add` добавляет T031), src/render/**,
src/overlay/**, include/ovr/state/**, contracts/** (кроме чтения).
```
---

### T033 — Цикл кадров: `src/app`, CLI `ovr`, крюки `main.cpp`

```text
ID: T033
Milestone: M4. Зависимости: T007, T008, T012, T013, T014, T015, T016, T021.
Цель:
  Собрать единственный исполняемый наблюдатель: цикл read_frame -> parse_frame -> run_all ->
  present, CLI из §10.12 и перезагрузку конфига только между кадрами. Без `--once N` «CI зелёный»
  не доказывает, что цикл кадров вообще собирается.

Вход:
  - ARCHITECTURE.md:907 (§10.12) — CLI `ovr`: аргументы и коды возврата (цитата ниже).
  - ARCHITECTURE.md:974 (§10.16) — app::Context, register_features, app::run (цитата ниже).
  - ARCHITECTURE.md:936 (§10.14) — FrameSource: единственная «нечистая» часть кадра.
  - ARCHITECTURE.md:363 (§3.11) — FeatureRegistry::run_all; ARCHITECTURE.md:235 (§3.6) — примитивы.
  - ARCHITECTURE.md:264 (§3.7) — проекция: откуда берутся размеры окна для `--window`.
  - ARCHITECTURE.md:336 (§3.10) — Config: hot-reload между кадрами, без мьютексов.
  - ARCHITECTURE.md:584 (§8) — приёмка без хоста, GPU и монитора: всё на фикстурах.
  - ARCHITECTURE.md:955 (§10.15) — `kFrameSlots` объявляется здесь.
  - ARCHITECTURE.md:999 (§11) — E1, E4, E10, E18, E19, E21, E25, E28, E30, E35.

Выход:
  - src/app/app.hpp (Context, register_features, run — дословно из §10.16; заглушку T000
    переписывает целиком именно эта задача).
  - src/app/app.cpp (цикл кадров, перезагрузка конфига, подсчёт кадр/с, `--once`).
  - src/app/cli.hpp, src/app/cli.cpp (разбор аргументов ровно по §10.12; своих ключей нет).
  - src/app/CMakeLists.txt (цель `ovr`; линкует ovr_core, ovr_offsets, ovr_memory, ovr_state,
    ovr_render, ovr_overlay, ovr_features, ovr_config — и ничего больше до M6).
  - src/main.cpp (становится `return ovr::app::run(argc, argv);` + `register_features()`;
    больше в этом файле менять нечего).
  - tests/unit/test_app_cli.cpp (аргументы, коды возврата, `--once` на фикстурах).
  - tests/unit/test_app_loop.cpp (первый кадр, устаревший реестр, перезагрузка конфига).

Контракт (из ARCHITECTURE.md:907 и ARCHITECTURE.md:974):
  ovr [--config <path>] [--env <path>] [--build <n>] [--window <w>x<h>] [--once <n>] [--help]
  exit 0 = штатно; exit 1 = смещения устарели или снапшот не найден (причина в первой строке
  stderr); exit 2 = битый конфиг либо аргументы
  struct Context { backend; writer; offsets; cfg; features; input; }   // §10.16, дословно
  int run(int argc, char** argv);   void register_features(FeatureRegistry&, Context&);
  Один кадр = ровно один `read_frame()` (§10.14), затем `parse_frame`, затем `run_all`, затем
  present. Между кадрами: проверка mtime конфига (E18) и `is_stale()` (E4, E30). Внутри кадра —
  ни одной аллокации и ни одного обращения к памяти кроме FrameSource (P4, §5).
  `--once N` работает на фикстурах (FakeBackend + golden-дамп): это единственная приёмка цикла
  кадров в CI, где нет хоста, GPU и монитора (E28).

Крайние случаи: E1, E4, E10, E16, E18, E19, E21, E25, E28, E30, E35 (каталог ARCHITECTURE.md:999 §11).
Ограничения:
  - Никаких потоков, процессов, сокетов: один поток, один кадр за итерацией (§5, P9).
  - Никакого `sleep()` в цикле; пауза между кадрами — только relative timer из core/time.hpp.
  - `src/app/**` не включает `src/features/<имя>/**`: список фич живёт в `register_features()`
    в `src/main.cpp` (единственный общий файл).
  - До M6 в Context поле `input` всегда nullptr (T019 ещё нет): фичи не должны это замечать (E21).
DoD / тест:
  1) `./build/src/app/ovr --help` печатает список аргументов и выходит 0; `--once 3` на фикстуре
     выходит 0 и печатает ровно 3 строки кадра; неизвестный аргумент -> exit 2.
  2) E1: без `--env` и без `OVR_HOST_ENV` наблюдатель работает на фикстурах, `host_env::
     is_available() == false`, exit 0 и в stderr одна строка «имена хоста недоступны».
  3) E4/E30: реестр с другим `ClientVersion` -> ни одного обращения к памяти (счётчик FakeBackend
     не изменился), на экране одна строка «смещения устарели», exit 1 у `--once`.
  4) E10: первый кадр пустой -> в оверлее нет нулей в углу (0 примитивов), цикл не падает.
  5) E18/E19: конфиг подменён между кадрами -> новые значения видны со следующего кадра; файл
     удалён -> дефолты продолжают действовать, в лог не больше одной строки всего прогон.
  6) E28: тесты проходят на Linux без GPU/монитора/хоста; окно создаётся только в рантайме
     (`grep -rn "CreateWindow|glfw|d3d" src/app` -> пусто).
  7) E25: канал записи convar-ов включён только явным переключателем конфига; при выключенном —
     ноль записей (счётчик FakeBackend), даже если фича «просит» запись.
  8) E21: на машине без `/dev/uinput` и без сессии окна `create_uinput()` -> nullptr:
     `Context.input` пустой, `--once 3` всё равно печатает 3 строки кадра и выходит 0,
     ни одного исключения и ни одной попытки нажать клавишу (тест на fake-бэкенде T019);
  9) E35: фикстура на 400 агентов -> бенч T015 печатает все пять этапов таблицы без
     `not-measured` (features = пустой реестр + синтетическая нагрузка kDrawReserve примитивов),
     `total <= 1.800 мс`, аллокаций в steady-кадре ноль.
  10) `grep -c "kFrameSlots" src/app/app.cpp` == 1; `grep -rn "sleep\|std::thread\|fork" src/app` -> пусто.
  11) E16: `markers_enabled == false` -> за кадр 0 примитивов, при этом чтение кадров и hot-reload конфига продолжают работать.
  12) `cmake --build build && ctest --test-dir build --output-on-failure` — зелёные без хоста.
Не трогать: include/ovr/state/**, include/ovr/render/**, src/features/** (кроме чтения),
src/memory/**, contracts/**.
```
---

# M5 — read-only фичи: рисуем то, что уже есть в HostState

Одна директория на фичу, один `IFeature` (ARCHITECTURE.md:363), один тест, одна строка
`registry.add`. Данные — только из `HostState` (ARCHITECTURE.md:211), рисование — только
примитивами `FrameDraw` (ARCHITECTURE.md:235) и проекцией `world_to_screen` (ARCHITECTURE.md:264).
Реализм фич — RESEARCH.md:218 (§8), имена настроек — RESEARCH.md:245, открытые вопросы —
RESEARCH.md:335 (§13). Почти всё зависит от T011: пока Q1/Q2 не закрыты, фича рисует «не знаю».
Ключи конфига заранее объявлены T021 (PRINCIPLES P6): у фичи есть `<имя_фичи>.enabled` и её
параметры из схемы, master-switch меток — `markers_enabled`. Если нужного ключа в схеме нет,
исполнитель возвращает «контракт неполон» (владелец ключей — T021), а не придумывает имя.

### T031 — Метки акторов и модель видимости (`vision`)

```text
ID: T031
Milestone: M5. Зависимости: T007, T011, T012, T013, T016, T021, T033.
Цель:
  Первая настоящая фича — метки акторов — и единственное место, где живёт гипотеза о тумане
  войны (§10.10). Если Q3 (RESEARCH.md:335) не подтвердится, меняется один файл этой задачи,
  а не восемь фич M5.

Вход:
  - ARCHITECTURE.md:363 (§3.11) — IFeature: что фиче разрешено и запрещено.
  - ARCHITECTURE.md:872 (§10.10) — IVisionModel: контракт модели видимости (цитата ниже).
  - ARCHITECTURE.md:235 (§3.6) — примитивы; ARCHITECTURE.md:264 (§3.7) — проекция мир->экран.
  - ARCHITECTURE.md:336 (§3.10) — ключи markers_enabled, marker_box_color, marker_distance_units.
  - ARCHITECTURE.md:211 (§3.5) — ActorSnapshot.visible_to_us; ARCHITECTURE.md:716 (§10.3) — Agent.
  - ARCHITECTURE.md:907 (§10.12) и ARCHITECTURE.md:999 (§11) — как фичу видно в `--once N`.
  - RESEARCH.md:183 (§7), ARCHITECTURE.md:501 (§6 R4) — метки не принимаются без view-полей.
  - RESEARCH.md:322 (§12) — что такое «видимость» и почему она требует модель видимости (риск R5).

Выход:
  - include/ovr/features/vision_model.hpp (IVisionModel из §10.10 дословно; заголовок публичный,
    чтобы T025/T026/T029 не включали чужую директорию фичи — §4.2).
  - src/features/vision/vision_model_null.cpp (реализация `null_vision_model()`:
    `established() == false`, `is_visible()` отвечает по ActorSnapshot.visible_to_us и больше ни
    по чему; угадывать форму сетки видимости запрещено, R5).
  - src/features/vision/vision_feature.hpp, src/features/vision/vision_feature.cpp
    (класс VisionFeature : public IFeature с name() == "vision").
  - src/features/vision/CMakeLists.txt (цель ovr_feature_vision: ovr_core + ovr_state +
    ovr_render + ovr_features + ovr_config).
  - src/main.cpp (ровно одна строка `registry.add(...VisionFeature...);` в register_features()).
  - tests/unit/test_vision.cpp (фича на подставном HostState, без хоста и без GPU).

Контракт (из ARCHITECTURE.md:363 и ARCHITECTURE.md:872):
  class IFeature { virtual std::string_view name() const=0;
                   virtual void run(const state::HostState&, const config::Config&, render::FrameDraw&)=0; };
  class IVisionModel {
  public:
    virtual ~IVisionModel() = default;
    virtual bool is_visible(const state::HostState&, std::uint32_t raw_index, int team) const = 0;
    virtual bool established() const = 0;    // false => «не знаю», 0 примитивов
  };
  const IVisionModel& null_vision_model();    // реализация по умолчанию
  Метки рисуются только примитивами §3.6 (рамка-rect, полоса состояния, имя, дистанция в агентах);
  мировой перевод строго через world_to_screen (T013), своих формул нет. Видимость актора —
  `ActorSnapshot.visible_to_us` (это подтверждённое поле §3.5), а не догадка о сетке видимости: модель нужна
  тем, кому нужен «кто кого видит» (T025, T026, T029). `markers_enabled == false` => возврат до
  первой отрисовки (0 примитивов, E16).

Крайние случаи: E12, E34, E35, E36 (каталог ARCHITECTURE.md:999 §11).
Ограничения:
  - Не рисовать каркас, линии приказа и кольца — это T024 и T025.
  - `IVisionModel` не должен знать ни про рендер, ни про память: вход — только HostState (§4.2).
  - `run()` без аллокаций: тест со счётным аллокатором; reserve() делает цикл, а не фича.
  - Пока view-поля Q2 не добыты, проекция даёт false (E12) и фича рисует 0 примитивов — это
    честный ответ, а не выдуманная матрица (P10).
DoD / тест:
  1) «два актора в 400 агентах, один невидимый»: включённые метки дают ровно 3 набора
     примитивов для видимого (rect + text + text) и 0 для невидимого; дистанция в подписи
     == 400 (+/-1) при известной матрице.
  2) E16: `markers_enabled == false` -> items().empty() при непустом HostState и ни одного
     обращения к модели (счётчик вызовов в подставной модели).
  3) E12: `View::valid == false` (view-полей нет) -> 0 примитивов и в описании результата
     «не установлено», а не посчитанная матрица.
  4) E36: у поля метки в дампе not-established -> 0 примитивов по этому полю, падений нет.
  5) E34: пустой HostState -> 0 примитивов, 0 аллокаций.
  6) E35: 400 агентов -> примитивов не больше kDrawReserve (§10.15), время run() в бюджете §5
     (проверяется на фикстуре, не на живом хосте).
  7) null-модель: `established() == false`; `is_visible()` совпадает с visible_to_us фикстуры
     на 1000 псевдослучайных индексов; out-of-range raw_index -> false без исключений.
  8) `grep -n "registry.add" src/main.cpp` — ровно одна строка (первая фича; git-коммитов не делать).
  9) общее правило 14 (до T023 рёбра include проверяются вручную по §4.2): src/features/vision/**
     не включает memory/** и render/**-бэкенды; vision_model.hpp не включает ничего, кроме state/.
  10) `cmake --build build && ctest --test-dir build --output-on-failure` — зелёные без хоста.
Не трогать: src/render/**, src/overlay/**, include/ovr/state/**, contracts/** (кроме чтения),
src/features/feature_registry.cpp (это T016).
```
---

### T017 — Индикатор добивания

```text
ID: T017
Milestone: M5. Зависимости: T007, T011, T012, T013, T016, T021, T033.
Цель:
  Индикатор добивания пажа: полоса состояния плюс отметка порога отказа от добивания. Точную оценку
  входящего урона не выдумывать: таблица урона в сборке 6985 не подтверждена
  (RESEARCH.md:335 (§13) Q4), поэтому фича отдаёт индикатор, а не вердикт «добить можно /
  нельзя».
Вход:
  - ARCHITECTURE.md:363 (§3.11) — IFeature/FeatureRegistry: что фиче можно и чего нельзя.
  - ARCHITECTURE.md:211 (§3.5) — HostState, Agent, validity_bits: единственный источник данных.
  - ARCHITECTURE.md:235 (§3.6) — FrameDraw: чем рисовать примитивы.
  - ARCHITECTURE.md:264 (§3.7) — world_to_screen: как мировая точка становится экранной.
  - ARCHITECTURE.md:336 (§3.10) — last_hit_helper.{enabled,indicator_color,show_deny_threshold}.
  - ARCHITECTURE.md:419 (§4.1) — файлы задачи: src/features/last_hit/ и tests/unit/test_last_hit.cpp.
  - ARCHITECTURE.md:444 (§4.2) — какие include-рёбра запрещены.
  - RESEARCH.md:218 (§8) — что реально делает индикатор добивания в существующих решениях.
  - RESEARCH.md:335 (§13, Q4) — почему без таблицы урона только индикатор.
Выход:
  - src/features/last_hit/last_hit_model.hpp, src/features/last_hit/last_hit_model.cpp — чистая
    политика: на входе Agent + render::View + флаги конфига, на выходе std::optional<Indicator>;
    без memory/ и без рисования.
  - src/features/last_hit/last_hit_feature.hpp, src/features/last_hit/last_hit_feature.cpp —
    адаптер IFeature, name() == "last_hit".
  - src/main.cpp — одна строка registry.add(...) в конец register_features().
  - tests/unit/test_last_hit.cpp — единственный тест задачи.
Контракт (из ARCHITECTURE.md:363):
  class IFeature {
    virtual std::string_view name() const = 0;
    virtual void run(const state::HostState&, const config::Config&, render::FrameDraw&) = 0;
  };
  last_hit_helper.enabled == false -> run() не добавляет ни одного примитива.
  На одного пажа ровно 3 примитива: rect-подложка, rect заполнения (ширина пропорциональна
  health/max_health), text с числом health.
  last_hit_helper.show_deny_threshold == true -> ровно 4-й примитив: text со строкой "?" —
  точное число порога отказа без таблицы урона не установлено (Q4).
  world_to_screen вернул false (w <= 0, точка вне окна) -> паж пропускается целиком.
  Время брать только из HostState::current_time / tick_client / tick_server.
Крайние случаи: E14, E34, E36 (каталог ARCHITECTURE.md:999 §11).
Ограничения:
  - Оценку входящего урона не выдумывать: таблица урона не подтверждена (RESEARCH.md:335 (§13) Q4).
  - Только пажи и нейтральные агенты; акторы рисуются в src/features/vision/ (T016), дублировать не нужно.
  - max_health == 0 или снятый validity-бит -> паж пропускается; деления на ноль быть не должно.
  - Чтений памяти в фиче нет: health/max_health берутся из HostState (T007/T008).
  - Convar-ы фича не меняет (ARCHITECTURE.md:363); писать в них умеет только T022.
  - В run() ноль аллокаций: буферы готовятся в конструкторе либо через reserve().
  - Новых ключей конфига не вводить; не хватает — вернуть «контракт неполон» (владелец T021).
DoD / тест:
  1) Фикстура: паж health/max_health = 30/100 -> ровно 3 примитива; ширина rect заполнения
     равна 30% ширины подложки с погрешностью +/-1 пиксель.
  2) Тот же паж и show_deny_threshold == true -> ровно 4 примитива, последний — text со строкой "?".
  3) last_hit_helper.enabled == false -> draw.items().empty() == true.
  4) Фикстура «за камерой» (w <= 0) -> 0 примитивов, исключений нет.
  5) Тест со счётным аллокатором: после первого кадра run() даёт 0 аллокаций.
  6) E34: пустой `HostState` (0 акторов, 0 агентов) -> 0 примитивов и 0 аллокаций.
  7) E36: у поля фичи в дампе `not-established` -> 0 примитивов и в отчёте «не установлено» (P10).
  8) E14: `max_health == 0` -> паж пропущен, бит достоверности снят, деления на ноль и NaN нет.
  9) grep -n "registry.add" src/main.cpp — строк registry.add стало на одну больше, чем до начала задачи; git-коммиты не делать.
  10) python3 scripts/check_includes.py (общее правило 14: до T023 рёбра include проверяются вручную по §4.2):
     src/features/last_hit/** не включает memory/ и render-бэкенды.
  11) cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j &&
     ctest --test-dir build --output-on-failure — зелёные без хоста.
Не трогать: src/features/vision/**, src/render/**, src/overlay/**, include/ovr/state/**, contracts/** (кроме чтения).
```

### T018 — Таймеры: метки времени, стоянки и скопления

```text
ID: T018
Milestone: M5. Зависимости: T007, T011, T012, T013, T016, T021, T033.
Цель:
  Board таймеров: для каждой наблюдаемой сущности (метка, стоянка, точка накопления) показать, сколько
  осталось до следующего появления. Интервал не берётся из вики — он измеряется по тикам
  HostState; константы хоста вида «метки каждые 180 секунд» не выдумывать.
Вход:
  - ARCHITECTURE.md:363 (§3.11) — IFeature: фича не читает память и не аллоцирует в run().
  - ARCHITECTURE.md:211 (§3.5) — HostState: tick_client/tick_server/current_time, agents, validity_bits.
  - ARCHITECTURE.md:235 (§3.6) — FrameDraw: примитивы, reserve(), порядок вставки.
  - ARCHITECTURE.md:264 (§3.7) — world_to_screen: привязка слота к экранной позиции.
  - ARCHITECTURE.md:336 (§3.10) — vision_tracker.beacon_timer_overlay как master-switch board'а.
  - ARCHITECTURE.md:419 (§4.1) — файлы задачи: src/features/timers/ и tests/unit/test_timers.cpp.
  - ARCHITECTURE.md:444 (§4.2) — запрещённые include-рёбра.
  - RESEARCH.md:218 (§8) — таймеры меток/стоянок/скоплений как реальная фича.
  - RESEARCH.md:335 (§13) — почему часть полей помечена not-established.
Выход:
  - src/features/timers/timer_board.hpp, src/features/timers/timer_board.cpp — чистый
    наблюдатель: каждый кадр получает span по агентам и обновляет фиксированный массив слотов
    (capacity 256) без кучи; состояние доступно только через геттеры board().
  - src/features/timers/timers_feature.hpp, src/features/timers/timers_feature.cpp — адаптер
    IFeature, name() == "timers".
  - src/main.cpp — одна строка registry.add(...) в конец register_features().
  - tests/unit/test_timers.cpp — единственный тест задачи.
Контракт (из ARCHITECTURE.md:363):
  class IFeature {
    virtual std::string_view name() const = 0;
    virtual void run(const state::HostState&, const config::Config&, render::FrameDraw&) = 0;
  };
  struct Slot {
    std::uint64_t signature;                     // ключ слота: одна подпись на одну сущность
    int observations;                            // сколько появлений наблюдено
    std::int64_t median_interval_ticks;          // медиана измеренных интервалов
    std::optional<std::int64_t> predicted_tick;  // пусто == not-established
  };
  Интервал = наблюдаемое число тиков между двумя появлениями; константы из вики не используются.
  Пока observations < 3, predicted_tick == nullopt (not-established); дальше == медиана +/-1 тик.
  Смена signature у слота -> слот сбрасывается (observations = 0, predicted_tick = nullopt).
  На активный слот с прогнозом ровно 2 примитива: rect прогресса + text вида "+12s".
  На активный слот без прогноза ровно 1 примитив: rect прогресса, без текста.
  vision_tracker.beacon_timer_overlay == false -> ни одного примитива.
Крайние случаи: E34, E36, E44 (каталог ARCHITECTURE.md:999 §11).
Ограничения:
  - Хранить только собственные числа и тики; указатели и raw-индексы хоста между кадрами не держать.
  - Wall-clock (std::chrono) запрещён: всё продвижение только от tick_client/tick_server.
  - capacity == 256: 257-я сущность не должна вызывать аллокаций; переполнение — счётчик,
    доступный через board().overflow_count().
  - Отдельных ключей конфига для меток/стоянок/скоплений в образце RESEARCH.md:245 нет -> вернуть
    «контракт неполон» (владелец ключей T021, P6); master-switch — vision_tracker.beacon_timer_overlay.
  - Свой рисовалки-бэкенд не заводить: только FrameDraw.
DoD / тест:
  1) Фикстура: сущность появилась на тике 100 и исчезла на тике 120 -> observations == 1,
     predicted_tick == nullopt, ровно 1 примитив.
  2) Три появления с интервалом 100 тиков -> predicted_tick == тик третьего появления + 100
     (+/-1 тик), ровно 2 примитива, text оканчивается на "s".
  3) 257 сущностей при capacity 256 -> после первого кадра 0 аллокаций, board().overflow_count() == 1.
  4) vision_tracker.beacon_timer_overlay == false -> draw.items().empty() == true.
  5) Смена signature у сущности -> её слот сброшен (observations == 0), прогноз снят.
  6) Повторный run() с тем же tick_client не меняет ни observations, ни predicted_tick.
  7) E34: пустой `HostState` (0 акторов, 0 агентов) -> 0 примитивов и 0 аллокаций.
  8) E36: у поля фичи в дампе `not-established` -> 0 примитивов и в отчёте «не установлено» (P10).
  9) grep -n "registry.add" src/main.cpp — на одну строку больше, чем до начала задачи;
     python3 scripts/check_includes.py (или вручную по §4.2:404): src/features/timers/** не тянет
     memory/** и render-бэкенды; git-коммиты не делать.
  10) cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j &&
     ctest --test-dir build --output-on-failure — зелёные без хоста.
  11) E44: `current_time` в фикстуре стоит на месте либо отмотан назад -> таймеры меток и
       стоянок не пересчитываются задом наперёд, остаток не уходит в минус, примитивов больше не становится.

Не трогать: src/features/vision/**, src/render/**, src/overlay/**, include/ovr/state/**, contracts/** (кроме чтения).
```

### T024 — Каркас: пары костей агента

```text
ID: T024
Milestone: M5. Зависимости: T007, T011, T012, T013, T016, T021, T033.
Цель:
  Костяк агента: набор отрезков между парами костей, спроецированных на экран. Таблица пар
  костей — это данные, а не код: она ссылается на имена полей из реестра T011. Сами m_*-поля
  в сборке 6985 не подтверждены (RESEARCH.md:335 (§13) Q1), поэтому до закрытия T011 фича
  рисует «не знаю» (ноль примитивов), а не угаданный каркас.
Вход:
  - ARCHITECTURE.md:363 (§3.11) — IFeature/FeatureRegistry: что фиче можно и чего нельзя.
  - ARCHITECTURE.md:211 (§3.5) — HostState, Agent, validity_bits: единственный источник данных.
  - ARCHITECTURE.md:235 (§3.6) — FrameDraw: чем рисовать line.
  - ARCHITECTURE.md:264 (§3.7) — world_to_screen: как мировая точка становится экранной.
  - ARCHITECTURE.md:336 (§3.10) — Config: чтение настроек; своих ключей у фичи нет.
  - ARCHITECTURE.md:419 (§4.1) — файлы задачи: src/features/skeleton/ и tests/unit/test_skeleton.cpp.
  - ARCHITECTURE.md:444 (§4.2) — какие include-рёбра запрещены.
  - RESEARCH.md:183 (§7) — какие view-поля нужны меткам и что из них подтверждено.
  - RESEARCH.md:218 (§8) — skeleton/joints в существующих решениях.
  - RESEARCH.md:335 (§13, Q1) — почему без m_*-полей каркас не рисуется.
Выход:
  - src/features/skeleton/skeleton_bones.hpp — таблица пар костей как данные: записи вида
    {from_slot, to_slot, имя_поля_из_реестра}; ни одного числа-смещения в исходнике.
  - src/features/skeleton/skeleton_model.hpp, src/features/skeleton/skeleton_model.cpp — чистая
    политика: на входе Agent + render::View + таблица пар, на выходе список экранных отрезков;
    без memory/ и без рисования.
  - src/features/skeleton/skeleton_feature.hpp, src/features/skeleton/skeleton_feature.cpp —
    адаптер IFeature, name() == "skeleton".
  - src/main.cpp — одна строка registry.add(...) в конец register_features().
  - tests/unit/test_skeleton.cpp — единственный тест задачи.
Контракт (из ARCHITECTURE.md:363):
  class IFeature {
    virtual std::string_view name() const = 0;
    virtual void run(const state::HostState&, const config::Config&, render::FrameDraw&) = 0;
  };
  markers_enabled == false -> run() не добавляет ни одного примитива.
  Одна пара костей, для обеих точек world_to_screen вернул true -> ровно 1 line.
  Таблица пар пуста (поля не подтверждены) -> 0 примитивов и в описании результата
  пометка not-established.
  Проецировать точки можно только через world_to_screen (ARCHITECTURE.md:264); если для пары
  хотя бы одна точка вернула false (w <= 0, вне окна) -> пара пропускается целиком.
  Время брать только из HostState::current_time / tick_client / tick_server.
Крайние случаи: E34, E36, E45 (каталог ARCHITECTURE.md:999 §11).
Ограничения:
  - Пары костей описывать ссылкой на имена полей T011, а не числами: литералы смещений
    >= 100000 запрещены (ARCHITECTURE.md:175).
  - Имя в реестре фиксировано: name() == "skeleton", переименовывать нельзя.
  - Ключи только объявленные T021 (P6): master-switch markers_enabled (RESEARCH.md:245);
    отдельный ключ цвета/толщины -> «контракт неполон», схему правит только T021.
    отдельная настройка цвета/толщины -> вернуть «контракт неполон», владелец ключей T021.
  - Convar-ы фича не меняет (ARCHITECTURE.md:363); писать в них умеет только T022.
  - В run() ноль аллокаций: буферы отрезков готовятся в конструкторе либо через reserve().
DoD / тест:
  1) Фикстура: агент + таблица из 3 пар костей, все точки внутри кадра -> ровно 3 line.
  2) Таблица пар пуста -> draw.items().empty() == true, в описании результата not-established.
  3) У одной из пар одна точка за камерой (w <= 0) -> ровно 2 line, исключений нет.
  4) markers_enabled == false -> draw.items().empty() == true.
  5) Тест со счётным аллокатором: после первого кадра run() даёт 0 аллокаций.
  6) E34: пустой `HostState` (0 акторов, 0 агентов) -> 0 примитивов и 0 аллокаций.
  7) E36: у поля фичи в дампе `not-established` -> 0 примитивов и в отчёте «не установлено» (P10).
  8) grep -n "registry.add" src/main.cpp — строк registry.add стало на одну больше, чем до начала задачи; git-коммиты не делать.
  9) python3 scripts/check_includes.py (общее правило 14: до T023 рёбра include проверяются вручную по §4.2):
     src/features/skeleton/** не включает memory/ и render-бэкенды.
  10) cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j &&
     ctest --test-dir build --output-on-failure — зелёные без хоста.
  11) E45: индекс кости вне диапазона и обнулённый указатель кости -> сегмент не рисуется,
       бит достоверности снят, остальные кости этого агента на месте.

Не трогать: src/features/vision/**, src/render/**, src/overlay/**, include/ovr/state/**, contracts/** (кроме чтения).
```

### T025 — Snap lines: линия до цели приказа

```text
ID: T025
Milestone: M5. Зависимости: T007, T011, T012, T013, T016, T021, T031, T033.
Цель:
  Линия от выбранного агента к объекту его текущего приказа (цель атаки/перемещения).
  Проверку «цель нас видит» делать не трассировкой луча по геометрии карты, а через модель
  видимости T031 (§10.10): она работает на данных сетки видимости (RESEARCH.md:335 (§13) Q3), а не на предположении,
  что код хоста можно воспроизвести снаружи.
Вход:
  - ARCHITECTURE.md:363 (§3.11) — IFeature/FeatureRegistry.
  - ARCHITECTURE.md:211 (§3.5) — HostState, Agent, order-поля, validity_bits.
  - ARCHITECTURE.md:235 (§3.6) — FrameDraw: line.
  - ARCHITECTURE.md:264 (§3.7) — world_to_screen для обоих концов линии.
  - ARCHITECTURE.md:336 (§3.10) — Config; своих ключей у фичи нет.
  - ARCHITECTURE.md:419 (§4.1) — src/features/snap_lines/ и tests/unit/test_snap_lines.cpp.
  - ARCHITECTURE.md:444 (§4.2) — запрещённые include-рёбра.
  - RESEARCH.md:218 (§8) — snap lines + visibility check в существующих решениях.
  - RESEARCH.md:335 (§13, Q3) — статус видимости partial: отвечать «не знаю», а не «не видно».
Выход:
  - src/features/snap_lines/snap_lines_model.hpp, src/features/snap_lines/snap_lines_model.cpp —
    чистая политика: HostState + агент + модель видимости T016 + render::View ->
    std::optional<Segment>; без memory/ и без рисования.
  - src/features/snap_lines/snap_lines_feature.hpp, src/features/snap_lines/snap_lines_feature.cpp —
    адаптер IFeature, name() == "snap_lines".
  - src/main.cpp — одна строка registry.add(...) в конец register_features().
  - tests/unit/test_snap_lines.cpp — единственный тест задачи.
Контракт (из ARCHITECTURE.md:363):
  class IFeature {
    virtual std::string_view name() const = 0;
    virtual void run(const state::HostState&, const config::Config&, render::FrameDraw&) = 0;
  };
  markers_enabled == false -> run() не добавляет ни одного примитива.
  У агента есть цель приказа и обе точки проектируются -> ровно 1 line между ними.
  Видимость цели проверять только вызовом is_visible(const HostState&, raw_index, team) из T016;
  своих формул видимости и обращения к памяти в фиче быть не должно.
  null-модель видимости (T016, not-established) -> 0 line и снятый validity-бит у цели.
  Любой из концов с world_to_screen == false -> для этого агента 0 line.
Крайние случаи: E34, E36, E46 (каталог ARCHITECTURE.md:999 §11).
Ограничения:
  - Поле цели приказа берётся из T011; если оно не подтверждено -> not-established и 0 line.
  - Имя в реестре фиксировано: name() == "snap_lines".
  - Ключи только объявленные T021 (P6): master-switch markers_enabled; отдельный ключ -> вернуть
    «контракт неполон», а не дописывать его в config.schema.json самому.
    «контракт неполон», владелец T021.
  - Модель видимости подключается только как заголовок include/ovr/features/vision_model.hpp
    (T031, §10.10): своих догадок о форме сетки видимости в фиче нет (R5).
  - Convar-ы не менять; в run() ноль аллокаций.
DoD / тест:
  1) Фикстура: агент + цель приказа, обе точки в кадре, is_visible == true -> ровно 1 line.
  2) null-модель видимости -> draw.items().empty() == true (это «не знаю», а не «не видно»).
  3) Цель за камерой (w <= 0) -> draw.items().empty() == true, исключений нет.
  4) markers_enabled == false -> draw.items().empty() == true.
  5) Тест со счётным аллокатором: после первого кадра run() даёт 0 аллокаций.
  6) E34: пустой `HostState` (0 акторов, 0 агентов) -> 0 примитивов и 0 аллокаций.
  7) E36: у поля фичи в дампе `not-established` -> 0 примитивов и в отчёте «не установлено» (P10).
  8) grep -n "registry.add" src/main.cpp — строк registry.add стало на одну больше, чем до начала задачи; git-коммиты не делать.
  9) python3 scripts/check_includes.py (общее правило 14: до T023 рёбра include проверяются вручную по §4.2):
     src/features/snap_lines/** не включает memory/ и render-бэкенды.
  10) cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j &&
     ctest --test-dir build --output-on-failure — зелёные без хоста.
  11) E46: цель приказа пропала между кадрами -> линия не рисуется, состояние фичи не «плывёт»,
       повторная цель следующего кадра обрабатывается как новая.

Не трогать: src/features/vision/**, src/render/**, src/overlay/**, include/ovr/state/**, contracts/** (кроме чтения).
```

### T026 — Fog overlay: сетка «кто кого видит»

```text
ID: T026
Milestone: M5. Зависимости: T007, T011, T012, T013, T016, T021, T031, T033.
Цель:
  Полупрозрачная сетка поверх карты: для каждой клетки статус «видно нам / видно противнику /
  не знаю». Размер клетки — параметр чистой модели (число агентов), а не convar и не смещение.
  Аналог существующих `src-F`-решений (RESEARCH.md:218 §8); статус «не знаю» обязателен,
  потому что данные сетки видимости в сборке 6985 подтверждены лишь частично (RESEARCH.md:335 (§13) Q3).
Вход:
  - ARCHITECTURE.md:363 (§3.11) — IFeature/FeatureRegistry.
  - ARCHITECTURE.md:211 (§3.5) — HostState::map_width/map_height: отсюда размер сетки.
  - ARCHITECTURE.md:235 (§3.6) — FrameDraw: rect.
  - ARCHITECTURE.md:264 (§3.7) — world_to_screen: corners клетки -> экранного четырёхугольника.
  - ARCHITECTURE.md:336 (§3.10) — Config; своих ключей у фичи нет.
  - ARCHITECTURE.md:419 (§4.1) — src/features/fow_overlay/ и tests/unit/test_fow_overlay.cpp.
  - ARCHITECTURE.md:444 (§4.2) — запрещённые include-рёбра.
  - RESEARCH.md:218 (§8) — map vision / fog overlay в существующих решениях.
  - RESEARCH.md:335 (§13, Q3) — сетка видимости подтверждена частично: статус partial = «не знаю».
  - ARCHITECTURE.md:872 (§10.10) — IVisionModel (заголовок T031): как спрашивать
    «видно ли»; null-модель отвечает «не знаю», угадывать форму сетки видимости запрещено (R5).
Выход:
  - src/features/fow_overlay/fow_grid.hpp, src/features/fow_overlay/fow_grid.cpp — чистая
    модель: HostState + модель видимости T016 + размер клетки -> таблица статусов клеток;
    без memory/ и без рисования.
  - src/features/fow_overlay/fow_overlay_feature.hpp, src/features/fow_overlay/fow_overlay_feature.cpp —
    адаптер IFeature, name() == "fow_overlay".
  - src/main.cpp — одна строка registry.add(...) в конец register_features().
  - tests/unit/test_fow_overlay.cpp — единственный тест задачи.
Контракт (из ARCHITECTURE.md:363):
  class IFeature {
    virtual std::string_view name() const = 0;
    virtual void run(const state::HostState&, const config::Config&, render::FrameDraw&) = 0;
  };
  Число клеток = ceil(map_width / cell) * ceil(map_height / cell), где map_width/map_height —
  из HostState (ARCHITECTURE.md:211); cell — аргумент модели, по умолчанию 64.
  Ровно 1 rect на клетку со статусом «видно нам» или «видно противнику»; для клетки со
  статусом «не знаю» -> 0 rect.
  markers_enabled == false -> run() не добавляет ни одного примитива.
  Клетка целиком за камерой (все углы world_to_screen == false) -> 0 rect для этой клетки.
  Осевшие validity-биты HostState -> сетка пересчитывается, устаревшие статусы не рисовать.
Крайние случаи: E15, E34, E36 (каталог ARCHITECTURE.md:999 §11).
Ограничения:
  - Статус клетки получать только через is_visible(...) из T016; своих расчётов сетки видимости не писать.
  - Размер клетки — константа модели, не config: своего ключа в схеме T021 нет, а выдумывать
    новый ключ = «контракт неполон» (P6).
    ключ -> «контракт неполон» (владелец T021).
  - Не путать с T016: vision-фича рисует самих агентов, здесь рисуются только клетки.
  - Convar-ы не менять (fog_enable/fow_client_nofiltering трогает только T022).
  - В run() ноль аллокаций: таблица клеток аллоцируется один раз при смене размера карты.
DoD / тест:
  1) Фикстура карты 128x128 агентов при cell == 64 -> ровно 4 rect (все клетки известны).
  2) Те же данные, но null-модель видимости -> draw.items().empty() == true.
  3) Смешанная фикстура: 2 известные клетки и 1 неизвестная -> ровно 2 rect.
  4) markers_enabled == false -> draw.items().empty() == true.
  5) Тест со счётным аллокатором: после первого кадра run() даёт 0 аллокаций.
  6) E15: `map_width == 0` либо `map_height == 0` -> 0 rect, деления на ноль нет.
  7) E34: пустой `HostState` (0 акторов, 0 агентов) -> 0 примитивов и 0 аллокаций.
  8) E36: у поля фичи в дампе `not-established` -> 0 примитивов и в отчёте «не установлено» (P10).
  9) grep -n "registry.add" src/main.cpp — строк registry.add стало на одну больше, чем до начала задачи; git-коммиты не делать.
  10) python3 scripts/check_includes.py (общее правило 14: до T023 рёбра include проверяются вручную по §4.2):
     src/features/fow_overlay/** не включает memory/ и render-бэкенды.
  11) cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j &&
     ctest --test-dir build --output-on-failure — зелёные без хоста.
Не трогать: src/features/vision/**, src/render/**, src/overlay/**, include/ovr/state/**, contracts/** (кроме чтения).
```

### T027 — Range displays: кольца радиусов

```text
ID: T027
Milestone: M5. Зависимости: T007, T011, T012, T013, T016, T021, T033.
Цель:
  На экране рисуются кольца: радиус воздействия, радиус охраны стражей, дистанция рывка. Каталог —
  RESEARCH.md:229 (§8) «Custom Range Displays»: фича помечена R+W, W-вариант через convar
  дешевле. Здесь делается только R-часть (кольцо рисуем сами); запись `cv.range_display` —
  вне объёма блока, см. Ограничения.
Вход:
  - ARCHITECTURE.md:363 (§3.11) — IFeature/FeatureRegistry.
  - ARCHITECTURE.md:211 (§3.5) — HostState: pos агентов, camera_origin, view_matrix.
  - ARCHITECTURE.md:235 (§3.6) — FrameDraw: circle, text.
  - ARCHITECTURE.md:264 (§3.7) — world_to_screen: единственная проекция мир -> экран.
  - ARCHITECTURE.md:336 (§3.10) — Config; своих ключей у фичи нет.
  - ARCHITECTURE.md:419 (§4.1) — src/features/range_display/ и tests/unit/test_range_display.cpp.
  - ARCHITECTURE.md:444 (§4.2) — запрещённые include-рёбра.
  - RESEARCH.md:183 (§7) — почему без world_to_screen кольца физически не нарисовать.
  - RESEARCH.md:229 (§8) — Custom Range Displays: R+W, W через convar.
  - RESEARCH.md:245 (§8) — config.json: своих ключей для фичи нет.
  - RESEARCH.md:335 (§13, Q1/Q2) — полей воздействий в сборке 6985 может не быть.
Выход:
  - src/features/range_display/range_policy.hpp, src/features/range_display/range_policy.cpp —
    чистая модель: HostState + Config -> список колец (центр, радиус в агентах, цвет, подпись);
    без memory/ и без рисования.
  - src/features/range_display/range_display_feature.hpp, src/features/range_display/range_display_feature.cpp —
    адаптер IFeature, name() == "range_display".
  - src/main.cpp — одна строка registry.add(...) в конец register_features().
  - tests/unit/test_range_display.cpp — единственный тест задачи.
Контракт (из ARCHITECTURE.md:363):
  class IFeature {
    virtual std::string_view name() const = 0;
    virtual void run(const state::HostState&, const config::Config&, render::FrameDraw&) = 0;
  };
  struct Ring { core::Vec3 center; float radius_units; std::uint32_t color; std::string_view label; };
  void range_policy::rings(const state::HostState&, const config::Config&, std::vector<Ring>& out);
  Адаптер хранит out как член и вызывает rings() с тем же вектором каждый кадр (out.clear()
  внутри) -> после первого кадра ёмкость не растёт и run() не аллоцирует.
  Экранный радиус кольца адаптер считает так: A = world_to_screen(center),
  B = world_to_screen(center + radius_units * u), где u = правый вектор камеры из view_matrix
  (ARCHITECTURE.md:211); радиус окружности на экране = |A - B| в пикселях.
  Ровно 1 circle + ровно 1 text на одно кольцо. Если world_to_screen вернул false для A или B
  -> 0 примитивов для этого кольца. rings() пуст -> 0 примитивов всего.
  markers_enabled == false -> run() не добавляет ни одного примитива.
Крайние случаи: E34, E36, E47 (каталог ARCHITECTURE.md:999 §11).
Ограничения:
  - Радиусы (воздействие / охрана стража / рывок) задаются только именованными константами в
    range_policy.cpp, каждая с комментарием-источником; своих формул перспективы и FOV не писать.
  - Запись `cv.range_display` из фичи запрещена: в IFeature нет канала записи convar-а ->
    на вопрос про W-часть исполнитель отвечает «контракт неполон» (владелец T022).
    Read-only MVP из-за этого не блокируется.
  - Данные только из HostState; memory/ в src/features/** не включать (§4.2:404).
  - Ключи только из T021 (P6): markers_enabled + camera.*; своего ключа погоды в схеме нет ->
    «контракт неполон» (владелец ключей T021), диапазон 0..10 брать константой модели.
DoD / тест:
  1) Фикстура: кольцо radius_units == 400 -> экранный радиус совпадает с эталоном теста
     (эталон = те же два вызова world_to_screen), отклонение <= 1 px.
  2) Кольцо целиком за камерой (world_to_screen == false) -> draw.items().empty() == true.
  3) HostState без подходящих сущностей -> rings() пуст и 0 примитивов.
  4) 3 кольца в фикстуре -> ровно 6 примитивов (3 circle + 3 text), порядок = порядок в rings().
  5) Тест со счётным аллокатором: после первого кадра run() даёт 0 аллокаций.
  6) E34: пустой `HostState` (0 акторов, 0 агентов) -> 0 примитивов и 0 аллокаций.
  7) E36: у поля фичи в дампе `not-established` -> 0 примитивов и в отчёте «не установлено» (P10).
  8) grep -n "registry.add" src/main.cpp — строк registry.add стало на одну больше, чем до начала задачи; git-коммиты не делать.
  9) python3 scripts/check_includes.py (общее правило 14: до T023 рёбра include проверяются вручную по §4.2):
     src/features/range_display/** не включает memory/ и render-бэкенды.
  10) cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j &&
     ctest --test-dir build --output-on-failure — зелёные без хоста.
  11) E47: радиус 0, отрицательный и камера в центре кольца -> 0 сегментов либо минимум по
       именованной константе; inf/NaN в координатах примитивов нет.

Не трогать: src/features/vision/**, src/render/**, src/overlay/**, include/ovr/state/**, contracts/** (кроме чтения).
```

### T028 — Cooldown tracker: тайминги воздействий

```text
ID: T028
Milestone: M5. Зависимости: T007, T011, T012, T013, T016, T021, T033.
Цель:
  Над воздействиями врагов — строка «готов через N с». Каталог — RESEARCH.md:230 (§8)
  «Cooldown & Mana Tracker»: фича требует цепочки полей воздействий, поэтому MVP — машина состояний на
  тиках HostState, а не выдуманные длительности воздействий.
Вход:
  - ARCHITECTURE.md:363 (§3.11) — IFeature/FeatureRegistry.
  - ARCHITECTURE.md:211 (§3.5) — HostState: tick_client и pos агентов; тики монотонны.
  - ARCHITECTURE.md:235 (§3.6) — FrameDraw: text.
  - ARCHITECTURE.md:264 (§3.7) — world_to_screen: позиция подписи на экране.
  - ARCHITECTURE.md:336 (§3.10) — Config: vision_tracker.show_enemy_cooldowns.
  - ARCHITECTURE.md:419 (§4.1) — src/features/cooldowns/ и tests/unit/test_cooldowns.cpp.
  - ARCHITECTURE.md:444 (§4.2) — запрещённые include-рёбра.
  - RESEARCH.md:230 (§8) — `Cooldown & Mana Tracker` требует цепочки полей воздействий.
  - RESEARCH.md:245 (§8) — vision_tracker.show_enemy_cooldowns — единственный ключ фичи.
  - RESEARCH.md:335 (§13, Q2) — поля воздействий в сборке 6985 не подтверждены.
Выход:
  - src/features/cooldowns/cooldown_machine.hpp, src/features/cooldowns/cooldown_machine.cpp —
    чистая модель: поток сэмплов -> оценка времени готовности; без memory/ и без рисования.
  - src/features/cooldowns/cooldowns_feature.hpp, src/features/cooldowns/cooldowns_feature.cpp —
    адаптер IFeature, name() == "cooldowns".
  - src/main.cpp — одна строка registry.add(...) в конец register_features().
  - tests/unit/test_cooldowns.cpp — единственный тест задачи.
Контракт (из ARCHITECTURE.md:363):
  class IFeature {
    virtual std::string_view name() const = 0;
    virtual void run(const state::HostState&, const config::Config&, render::FrameDraw&) = 0;
  };
  struct AbilitySample { std::uint32_t raw_index; std::uint32_t slot; bool available; };
  void cooldown_machine::observe(std::uint64_t tick, std::span<const AbilitySample> samples);
  std::optional<std::uint64_t> cooldown_machine::ready_at(std::uint32_t raw_index,
      std::uint32_t slot) const;
  observe() вызывается ровно один раз за кадр, tick = HostState::tick_client (ARCHITECTURE.md:211).
  Переход true -> false запоминает тик применения; переход false -> true измеряет длительность
  воздействия и запоминает её как оценку для пары (raw_index, slot). Повторное применение на тике F ->
  ready_at == F + последняя измеренная длительность, погрешность +-1 тик.
  Если измеренной длительности для пары ещё нет -> ready_at возвращает std::nullopt, и адаптер
  рисует строку «???» вместо числа.
  Ровно 1 text на пару (raw_index, slot); позиция — world_to_screen от pos агента, false ->
  0 примитивов. markers_enabled == false или show_enemy_cooldowns == false -> 0 примитивов.
Крайние случаи: E34, E36, E44 (каталог ARCHITECTURE.md:999 §11).
Ограничения:
  - Длительности воздействий не хардкодить и не брать «из головы»: их в контракте нет
    (RESEARCH.md:335 (§13) Q2). Единственный источник — измеренные переходы available.
  - observe() не знает ни про память, ни про FrameDraw; вся логика времени — только в тиках,
    без wall-clock и без sleep.
  - Сэмплы адаптер строит из HostState; если полей воздействий там нет -> observe() вызывается с
    пустым span и 0 примитивов (не догадываться).
  - Не путать с T018 timers: там таймеры появления/скоплений, здесь — тайминги воздействий воздействий.
DoD / тест:
  1) Фикстура: available == false на тике 100, true на тике 160 -> измеренная длительность 60;
     снова false на тике 200 -> ready_at == 260 (+-1).
  2) Пара без измеренной длительности -> ready_at == std::nullopt и ровно 1 текст «???».
  3) 2 агента по 2 воздействия -> ровно 4 text; world_to_screen == false -> 0 примитивов.
  4) markers_enabled == false -> draw.items().empty() == true (при тех же сэмплах observe() продолжается).
  5) Тест со счётным аллокатором: после первого кадра run() даёт 0 аллокаций.
  6) E34: пустой `HostState` (0 акторов, 0 агентов) -> 0 примитивов и 0 аллокаций.
  7) E36: у поля фичи в дампе `not-established` -> 0 примитивов и в отчёте «не установлено» (P10).
  8) grep -n "registry.add" src/main.cpp — строк registry.add стало на одну больше, чем до начала задачи; git-коммиты не делать.
  9) python3 scripts/check_includes.py (общее правило 14: до T023 рёбра include проверяются вручную по §4.2):
     src/features/cooldowns/** не включает memory/ и render-бэкенды.
  10) cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j &&
     ctest --test-dir build --output-on-failure — зелёные без хоста.
  11) E44: время хоста стоит/отмотан -> остаток тайминга воздействий не уменьшается сам собой
       и не становится отрицательным; готовность пересчитывается по тикам (E22).

Не трогать: src/features/vision/**, src/render/**, src/overlay/**, include/ovr/state/**, contracts/** (кроме чтения).
```

### T029 — Обманки и зоны истинного обзора

```text
ID: T029
Milestone: M5. Зависимости: T007, T011, T012, T013, T016, T021, T031, T033.
Цель:
  Обманки (иллюзии/клоны) помечаются рамкой и подписью; зоны истинного обзора (предметы обзора) — кольцом,
  если поля подтверждены. Каталог — RESEARCH.md:233 (§8) «обманки и истинный обзор», деривация
  обманок через m_hPlayer.
Вход:
  - ARCHITECTURE.md:363 (§3.11) — IFeature/FeatureRegistry.
  - ARCHITECTURE.md:211 (§3.5) — HostState: Agent-записи, pos, team, flags, alive.
  - ARCHITECTURE.md:235 (§3.6) — FrameDraw: rect, circle, text.
  - ARCHITECTURE.md:264 (§3.7) — world_to_screen: обводка и кольцо.
  - ARCHITECTURE.md:336 (§3.10) — Config: vision_tracker.illusion_highlight.
  - ARCHITECTURE.md:419 (§4.1) — src/features/illusions/ и tests/unit/test_illusions.cpp.
  - ARCHITECTURE.md:444 (§4.2) — запрещённые include-рёбра.
  - RESEARCH.md:183 (§7) — проекция и требования к отрисовке меток.
  - RESEARCH.md:233 (§8) — обманки и предметы обзора: фича `src-D`.
  - RESEARCH.md:245 (§8) — vision_tracker.illusion_highlight — единственный ключ фичи.
  - RESEARCH.md:335 (§13, Q1/Q2) — флаги обманок и предметов обзора в сборке 6985 не подтверждены.
  - ARCHITECTURE.md:872 (§10.10) — IVisionModel (заголовок T031): как спрашивать
    «видно ли»; null-модель отвечает «не знаю», угадывать форму сетки видимости запрещено (R5).
Выход:
  - src/features/illusions/illusion_model.hpp, src/features/illusions/illusion_model.cpp —
    чистая модель: HostState -> raw-индексы обманок и центры зон истинного обзора; без memory/ и без
    рисования.
  - src/features/illusions/illusions_feature.hpp, src/features/illusions/illusions_feature.cpp —
    адаптер IFeature, name() == "illusions".
  - src/main.cpp — одна строка registry.add(...) в конец register_features().
  - tests/unit/test_illusions.cpp — единственный тест задачи.
Контракт (из ARCHITECTURE.md:363):
  class IFeature {
    virtual std::string_view name() const = 0;
    virtual void run(const state::HostState&, const config::Config&, render::FrameDraw&) = 0;
  };
  struct Illusion { std::uint32_t raw_index; core::Vec3 pos; int team; };
  struct Zone { core::Vec3 center; float radius_units; };
  void illusion_model::illusions(const state::HostState&, std::vector<Illusion>& out);
  void illusion_model::reveal_zones(const state::HostState&, std::vector<Zone>& out);
  Признаком обманки считается только явное поле/флаг из HostState (ARCHITECTURE.md:211). Если
  такого поля в контракте HostState нет -> illusions() возвращает пусто и адаптер не рисует
  ничего; эвристики по имени модели агента запрещены.
  Ровно 1 rect-обводка + ровно 1 text «ill» на обманку; ровно 1 circle на зону истинного обзора.
  world_to_screen вернул false -> 0 примитивов для этого объекта. markers_enabled == false ->
  0 примитивов всего; illusion_highlight == false -> обманки не рисуются, зоны остаются.
  Адаптер держит оба выходных вектора как члены -> после первого кадра run() не аллоцирует.
Крайние случаи: E34, E36 (каталог ARCHITECTURE.md:999 §11).
Ограничения:
  - Статус not-established не превращать в «рисуем всем подряд»: нет полей -> 0 примитивов и
    комментарий со ссылкой на RESEARCH.md:335 (§13) Q1/Q2.
  - m_hPlayer-деривацию реализует T011 (contracts/fields.client-6985.json); здесь только чтение
    готовых полей HostState, memory/ не трогать.
  - Радиусы зон истинного обзора — именованные константы с комментарием-источником, не из головы:
    если источника нет -> константа не вводится, зона не рисуется.
  - Ключи только объявленные T021 (P6): markers_enabled как master-switch плюс
    vision_tracker.illusion_highlight; своего ключа не вводить — «контракт неполон».
DoD / тест:
  1) Фикстура: 3 агента, 2 помечены обманками -> ровно 4 примитива (2 rect + 2 text).
  2) Фикстура без поля-признака -> illusions().empty() == true и 0 примитивов.
  3) Одна зона истинного обзора -> ровно 1 circle; та же зона за камерой -> 0 примитивов.
  4) illusion_highlight == false -> обманок 0, зон 1 (ровно 1 circle).
  5) Тест со счётным аллокатором: после первого кадра run() даёт 0 аллокаций.
  6) E34: пустой `HostState` (0 акторов, 0 агентов) -> 0 примитивов и 0 аллокаций.
  7) E36: у поля фичи в дампе `not-established` -> 0 примитивов и в отчёте «не установлено» (P10).
  8) grep -n "registry.add" src/main.cpp — строк registry.add стало на одну больше, чем до начала задачи; git-коммиты не делать.
  9) python3 scripts/check_includes.py (общее правило 14: до T023 рёбра include проверяются вручную по §4.2):
     src/features/illusions/** не включает memory/ и render-бэкенды.
  10) cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j &&
     ctest --test-dir build --output-on-failure — зелёные без хоста.
Не трогать: src/features/vision/**, src/render/**, src/overlay/**, include/ovr/state/**, contracts/** (кроме чтения).
```

### T030 — Алерты и W-политики (камера, погода)

```text
ID: T030
Milestone: M5. Зависимости: T007, T011, T012, T013, T016, T021, T033.
Цель:
  R: строка-алерт «агент в рывке уже летит» и кольцо «меня видит этот враг» для видимых врагов.
  W: две чистые политики — желанная дистанция камеры и желанная погода. Политики только
  возвращают число; никуда не пишут (канал записи — T022).
Вход:
  - ARCHITECTURE.md:363 (§3.11) — IFeature/FeatureRegistry.
  - ARCHITECTURE.md:211 (§3.5) — HostState: pos, team, flags, camera_origin.
  - ARCHITECTURE.md:235 (§3.6) — FrameDraw: text, circle.
  - ARCHITECTURE.md:264 (§3.7) — world_to_screen: якорь алерта и кольцо.
  - ARCHITECTURE.md:313 (§3.9) — для ясности: нажатия клавиш здесь запрещены (это T019).
  - ARCHITECTURE.md:336 (§3.10) — Config: camera.custom_distance (900..2000), camera.smooth_zoom.
  - ARCHITECTURE.md:419 (§4.1) — src/features/alerts/ и tests/unit/test_alerts.cpp.
  - ARCHITECTURE.md:444 (§4.2) — запрещённые include-рёбра.
  - RESEARCH.md:160 (§6) — `cv.camera_distance` дефолт 1200, r_farz = 2 * distance, cl_weather 0..10.
  - RESEARCH.md:236, :233, :234, :235 (§8) — camera distance / weather / charge alert / visible-by-enemy ring.
  - RESEARCH.md:245 (§8) — camera.* есть; отдельного ключа погоды в каталоге нет.
  - RESEARCH.md:335 (§13) — открытые вопросы по флагам сущностей.
Выход:
  - src/features/alerts/alert_policy.hpp, src/features/alerts/alert_policy.cpp — чистая модель:
    HostState + модель видимости T016 -> список алертов и колец; без memory/ и без рисования.
  - src/features/alerts/value_policy.hpp, src/features/alerts/value_policy.cpp — чистые функции
    clamp для камеры и погоды; ни HostState, ни память не читают сверх Config.
  - src/features/alerts/alerts_feature.hpp, src/features/alerts/alerts_feature.cpp — адаптер
    IFeature, name() == "alerts".
  - src/main.cpp — одна строка registry.add(...) в конец register_features().
  - tests/unit/test_alerts.cpp — единственный тест задачи.
Контракт (из ARCHITECTURE.md:363):
  class IFeature {
    virtual std::string_view name() const = 0;
    virtual void run(const state::HostState&, const config::Config&, render::FrameDraw&) = 0;
  };
  struct AlertLine { std::string_view text; core::Vec3 anchor_world; };
  void alert_policy::alerts(const state::HostState&, std::vector<AlertLine>& out);
  void alert_policy::visible_enemy_rings(const state::HostState&, const config::Config&,
      std::vector<core::Vec3>& out);
  float value_policy::desired_camera_distance(const config::Config&);
  int value_policy::clamp_weather(int requested);
  Алерт: ровно 1 text на элемент out; признак рывка — только по полям HostState, если их нет ->
  out пуст (не догадываться). Кольцо: ровно 1 circle на врага, для которого
  is_visible(...) из T016 вернул true; врагов с false и союзников не считать.
  Адаптер держит out как член: после первого кадра ёмкость не растёт, аллокаций ноль (P4).
  desired_camera_distance: camera.custom_distance зажимается в [900, 2000]
  (ARCHITECTURE.md:336); smooth_zoom == false -> значение возвращается сразу, анимаций нет.
  clamp_weather: зажим в [0, 10] (RESEARCH.md:160 §6 cl_weather).
  markers_enabled == false -> run() не добавляет ни одного примитива.
Крайние случаи: E34, E36 (каталог ARCHITECTURE.md:999 §11).
Ограничения:
  - Политики не пишут в память и не меняют convar-ы: канал записи — T022; r_farz = 2 * distance
    — побочный эффект записи, его T022 не трогать здесь (RESEARCH.md:160 §6).
  - Погодного ключа в RESEARCH.md:245 нет -> интеграцию clamp_weather с Config не выдумывать:
    функция принимает int, а привязка к конфигу -> «контракт неполон» (владелец T021).
  - Кольца считать только через is_visible(...) из T016; своей оценки сетки видимости не писать.
  - Никаких нажатий клавиш и мыши (§3.9:284 — это поверхность T019/T020).
DoD / тест:
  1) Фикстура: у врага установлен признак рывка -> ровно 1 text; без признака -> 0 примитивов.
  2) 3 видимых врага (is_visible == true) и 2 невидимых -> ровно 3 circle.
  3) custom_distance == 2500 -> desired_camera_distance() == 2000; custom_distance == 500 -> 900;
     1200 -> 1200.
  4) clamp_weather(-1) == 0, clamp_weather(11) == 10, clamp_weather(5) == 5.
  5) Тест со счётным аллокатором: после первого кадра run() даёт 0 аллокаций.
  6) E34: пустой `HostState` (0 акторов, 0 агентов) -> 0 примитивов и 0 аллокаций.
  7) E36: у поля фичи в дампе `not-established` -> 0 примитивов и в отчёте «не установлено» (P10).
  8) grep -n "registry.add" src/main.cpp — строк registry.add стало на одну больше, чем до начала задачи; git-коммиты не делать.
  9) python3 scripts/check_includes.py (общее правило 14: до T023 рёбра include проверяются вручную по §4.2):
     src/features/alerts/** не включает memory/ и render-бэкенды.
  10) cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j &&
     ctest --test-dir build --output-on-failure — зелёные без хоста.
Не трогать: src/features/vision/**, src/render/**, src/overlay/**, include/ovr/state/**, contracts/** (кроме чтения).
```

---

# M6 — Ввод, convar-ы и CI: то, что меняет хост

Здесь всё, что выходит за рамки чтения: нажатие клавиш (ARCHITECTURE.md:313 (§3.9)), запись
в convar-ы (RESEARCH.md:160 (§6)) и скрипты, которые держат контракты в актуальности
(RESEARCH.md:302 (§11)). Поверхность настроек не растёт: меню — T014, реестр фич — T016,
ключи конфига — только T021 (он сделан в M2, тип конфига дописывать не нужно). В хост идут
ровно два канала: клавиши через SendInput/uinput и convar-ы через белый список; всё
остальное остаётся read-only. Наводчик (RESEARCH.md:241) — в бэклог, ID ему не давать.

### T019 — Input-бэкенды: SendInput / uinput, белый список клавиш и fake для тестов

```text
ID: T019
Milestone: M6. Зависимости: T001, T004, T015, T021.
Цель:
  Дать фичам единственный легальный способ «нажать кнопку»: два реальных бэкенда (Windows и
  Linux), один фейковый для тестов и механический белый список клавиш. Итог — ни одна фича не
  может отправить в хост произвольный ввод: whitelist отсекает его до обращения к ОС.
Вход:
  - ARCHITECTURE.md:313 (§3.9) — точные сигнатуры Key, IInputBackend, create_sendinput/create_uinput.
  - ARCHITECTURE.md:770 (§10.5) — `core::Key`: enum клавиш один на проект, своего в `input/` нет.
  - ARCHITECTURE.md:810 (§10.7) — `IInputSource`: белый список до обращения к ОС (цитата ниже).
  - ARCHITECTURE.md:336 (§3.10) — Config: клавиши и задержки приходят из конфига, не зашиваются.
  - ARCHITECTURE.md:419 (§4.1) — include/ovr/input/ и src/input/ принадлежат T019 и T020.
  - ARCHITECTURE.md:444 (§4.2) — какие include-рёбра запрещены между слоями.
  - ARCHITECTURE.md:584 (§8) — тестируемость: реальные бэкенды не должны участвовать в модульный тестах.
  - RESEARCH.md:259 (§9) — перехваты и подсадка модулей в хост = риск; ввод только системными API без перехвата.
  - RESEARCH.md:346 (§13, Q8) — почему SendInput/uinput, а не запись в CInput.
Выход:
  - заголовка `include/ovr/input/key.hpp` НЕ создавать: `Key` один на весь проект и лежит в
    `include/ovr/core/key.hpp` (объявляет T001, §10.5); в `input/` только `using core::Key;` (§3.9).
  - include/ovr/input/input_backend.hpp — IInputBackend { press(Key), release(Key), is_down(Key) }.
  - include/ovr/input/input_source.hpp — IInputSource из §10.7 (ARCHITECTURE.md:810) дословно:
    обёртка над бэкендом + белый список `std::span<const core::Key>` +
    `create_input_source(IInputBackend*, std::span<const core::Key>)`.
  - src/input/sendinput.cpp — Windows-реализация, ВЕСЬ файл под #ifdef _WIN32.
  - src/input/uinput.cpp — Linux-реализация через /dev/uinput (udev-зависимость не добавлять).
  - src/input/fake_input.cpp — тестовый бэкенд со счётчиками press/release и таблицей is_down.
  - src/input/CMakeLists.txt — цель ovr_input: линкует только ovr_core и ovr_config.
  - tests/unit/test_input_whitelist.cpp, tests/unit/test_input_fake.cpp.
  - корневой CMakeLists.txt не трогать: guarded add_subdirectory(src/input) уже создан T000 (P5).
Контракт (из ARCHITECTURE.md:313):
  namespace ovr::input
  using core::Key;   // единый enum клавиш — §10.5 (владелец T001)
  class IInputBackend {
  public:
    virtual ~IInputBackend() = default;
    virtual bool press(Key) = 0;
    virtual bool release(Key) = 0;
    virtual bool is_down(Key) const = 0;
  };
  IInputBackend* create_sendinput();   // Windows
  IInputBackend* create_uinput();      // Linux
  class IInputSource {                 // §10.7, дословно
  public:
    virtual ~IInputSource() = default;
    virtual bool press(core::Key) = 0;       // false = клавиши нет в списке, бэкенд не тронут
    virtual bool release(core::Key) = 0;
    virtual bool is_down(core::Key) const = 0;
    virtual std::span<const core::Key> whitelist() const = 0;
  };
  std::unique_ptr<IInputSource> create_input_source(IInputBackend*, std::span<const core::Key>);
  Клавиша вне белого списка -> press/release возвращают false и в бэкенд НЕ обращаются.
  Клавиша внутри списка -> ровно один вызов бэкенда, возврат true.
  Фильтр один — в IInputSource (§10.7): бэкенд про список ничего не знает, иначе правило
  разъедется по двум реализациям (§12.3). Пустой список означает «ввод выключен», а не
  «разрешено всё» (P10). Список даёт Config (T021), а не код задачи.
  Недоступный бэкенд (нет /dev/uinput, нет сессии Win) -> фабрика возвращает nullptr,
  исключений не бросаем; вызывающий код работает без ввода.
  Задержек, потоков и sleep в бэкенде нет: тайминги — ответственность фичи (T020).
Крайние случаи: E20, E21 (каталог ARCHITECTURE.md:999 §11).
Ограничения:
  Никаких перехватов (SetWindowsHookEx, X11 record, eBPF) и записи в CInput (RESEARCH.md:259).
  src/input/** не включает memory/, render/, features/ (§4.2:404); windows.h — только внутри
  #ifdef _WIN32 в sendinput.cpp.
  Тесты не открывают /dev/uinput и не подают реальный ввод: проверяется только fake + whitelist.
  Свой ключ конфига не выдумывать: список клавиш фиче отдаёт вызывающий код из Config (T021).
DoD / тест:
  1) fake: press(K) -> счётчик press==1 и is_down(K)==true; release(K) -> 0/false.
  2) E20: белый список — K в списке -> 1 вызов бэкенда и true; K вне списка -> false и 0 вызовов бэкенда.
  3) пустой список -> любой Key даёт false, бэкенд не вызван ни разу.
  4) E21: фабрики на недоступном бэкенде -> nullptr без исключений (тест не требует root и GUI).
  5) grep -rn "sleep_for\|nanosleep\|Sleep(\|std::thread" src/input include/ovr/input — пусто.
  6) grep -rn "windows.h" src/input — только в sendinput.cpp и только внутри #ifdef _WIN32.
  7) python3 scripts/check_includes.py (общее правило 14: до T023 рёбра include проверяются вручную по §4.2):
     src/input/** не включает memory/, render/, features/.
  8) cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j &&
     ctest --test-dir build --output-on-failure — зелёные без хоста.
Не трогать: src/features/**, src/render/**, src/overlay/**, include/ovr/config/**, contracts/** (кроме чтения).
```

### T020 — Связка: машина состояний приказов (по умолчанию выключена)

```text
ID: T020
Milestone: M6. Зависимости: T007, T012, T016, T019, T021, T022, T033.
Цель:
  Самая рискованная фича каталога (RESEARCH.md:234) — связка из заранее описанной последовательности приказов. Делаем её максимально
  скудной: чистая машина состояний «нажать одну кнопку, выдержать паузу, не спамить», отделённая
  от ввода и от рисования, выключенный по умолчанию. Фича проверяет, что связка HostState ->
  политика -> input работает, и не лезет в специфику акторов.
Вход:
  - ARCHITECTURE.md:363 (§3.11) — IFeature/FeatureRegistry: что фиче можно и чего нельзя.
  - ARCHITECTURE.md:313 (§3.9) — IInputBackend/IInputSource: единственный способ нажать кнопку.
  - ARCHITECTURE.md:211 (§3.5) — HostState: current_time, тики, список агентов — источник времени.
  - ARCHITECTURE.md:235 (§3.6) — FrameDraw: чем показать, что фича сейчас делает (или молчит).
  - ARCHITECTURE.md:336 (§3.10) — combo.enabled, target_lock_key, cast_delay_ms (0..200).
  - ARCHITECTURE.md:419 (§4.1) — src/features/combo/ принадлежит T019/T020.
  - RESEARCH.md:234 (§8) — «Связка» как фича класса I (input) и её риск.
  - RESEARCH.md:346 (§13, Q8) — почему по умолчанию выключено и почему акторов с длинными цепочками не делаем.
Выход:
  - src/features/combo/combo_machine.hpp, combo_machine.cpp — чистая политика: на входе
    HostState-срез + Config-срез, на выходе решение «нажимать/ждать» и номер воздействия из
    белого списка; без memory/, без input/, без render/.
  - src/features/combo/combo_feature.hpp, combo_feature.cpp — адаптер IFeature,
    name() == "combo"; он единственный трогает IInputSource и FrameDraw.
  - src/main.cpp — одна строка registry.add(...) в конец register_features().
  - tests/unit/test_combo.cpp + tests/fixtures/combo/*.json (состояния HostState).
Контракт (из ARCHITECTURE.md:363):
  class IFeature {
    virtual std::string_view name() const = 0;
    virtual void run(const state::HostState&, const config::Config&, render::FrameDraw&) = 0;
  };
  combo.enabled == false (дефолт RESEARCH.md:346) -> за весь кадр ноль вызовов press().
  target_lock_key не нажата -> ноль press(); нажата и машина состояний разрешает приказ -> ровно одно нажатие.
  Два приказа подряд разнесены не менее чем на cast_delay_ms, время берётся из HostState, не из ОС.
  Наведение/выбор цели не угадывается: цель = агент из HostState, помеченный как валидный
  (ARCHITECTURE.md:211); валидных нет -> фича молчит и рисует not-established.
Крайние случаи: E22, E46 (каталог ARCHITECTURE.md:999 §11).
Ограничения:
  Белый список воздействий — только идентификаторы из конфига (RESEARCH.md:245); своих ключей
  не выдумывать, не хватает настройки -> «контракт неполон», владелец ключей T021.
  Специфика акторов с длинными цепочками приказов (RESEARCH.md:234) требует цепочки полей
  воздействий из дампа (Q1, T011):
  пока она не закрыта, реализуется универсальная одно-клавишная связка, а не связка под конкретного актора;
  дописывать таблицы акторов = «контракт неполон».
  Никаких sleep_for/потоков/таймеров: пауза считается из current_time и тиков HostState.
  Input — только через IInputSource из T019 (белый список обязателен), convar-ы не менять.
  В run() ноль аллокаций; внутреннее состояние фичи — её поля, создаваемые до первого кадра.
DoD / тест:
  1) enabled==true, клавиша нажата, пауза истекла -> ровно 1 press(); кадр сразу следом -> 0 press().
  2) enabled==false -> 600 кадров, fake-бэкенд насчитал 0 press(); в FrameDraw ни одного примитива связки.
  3) E22: cast_delay_ms==15 -> второй приказ не раньше current_time+15мс; cast_delay_ms==0 -> без паузы.
  4) combo_machine тестируется без memory/render/input (линкуется только с ovr_core+ovr_config);
     grep -rn "memory/\|render/\|input/" src/features/combo/combo_machine.* — пусто.
  5) Тест со счётным аллокатором: после первого кадра run() даёт 0 аллокаций.
  6) grep -rn "sleep_for\|std::thread\|WriteProcess\|convar" src/features/combo — пусто.
  7) python3 scripts/check_includes.py (общее правило 14: до T023 рёбра include проверяются вручную по §4.2):
     src/features/combo/** не включает memory/ и render-бэкенды.
  8) cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j &&
     ctest --test-dir build --output-on-failure — зелёные без хоста.
  9) E46: цель приказа исчезла между кадрами (raw_index больше нет в agents) -> приказ
       отменяется, за кадром ни одного press(); линия и индикатор не рисуются.

Не трогать: src/input/** (это T019), include/ovr/config/** (это T021), src/render/**, src/overlay/**, остальные src/features/**, contracts/** (кроме чтения).
```

### T022 — Convar-ы: таблица-белый список и идемпотентное восстановление

```text
ID: T022
Milestone: M6. Зависимости: T001, T004, T006, T021.
Цель:
  Закрепить за одной задачей всю запись в хост: таблица разрешённых convar-ов как данные, одна
  точка применения конфига и честное восстановление значений. Иначе W-фичи (T027, T030) начнут
  писать в память хоста кто куда хочет, и это главный источник ограничений доступа (RESEARCH.md:259).
Вход:
  - RESEARCH.md:160 (§6) — единственный источник: имена, риск, диапазоны, дефолты.
  - RESEARCH.md:179 — `cv.particle_fow` удалён в сборке 6985: писать в него нельзя.
  - RESEARCH.md:344 (§13, Q6) — cl_weather: валидируем только диапазон 0..10, имена не используем.
  - ARCHITECTURE.md:336 (§3.10) — Config как единственный источник значений (camera.*, weather).
  - ARCHITECTURE.md:363 (§3.11) — фича не меняет convar-ы: setter живёт в T022 и зовётся из `main`,
    отсюда «записи нет, пока фича явно не включена» (E25, риск R2).
  - ARCHITECTURE.md:830 (§10.8) — таблица convar-ов и сигнатуры apply_from_config/restore_all
    (цитата ниже дословно); ARCHITECTURE.md:787 (§10.6) — `IMemoryWriter`: чем писать.
  - ARCHITECTURE.md:955 (§10.15) — новых констант у этой задачи нет; ёмкости брать не откуда.
  - ARCHITECTURE.md:175 (§3.3) — OffsetRegistry: адреса ищутся по имени, литералы запрещены.
  - ARCHITECTURE.md:419 (§4.1) — src/convars/ и tests/unit/test_convar_whitelist.cpp = T022.
  - ARCHITECTURE.md:444 (§4.2) — в contracts/** пишет только T011; сюда — только чтением.
  - ARCHITECTURE.md:501 (§6) — риск R8: cl_weather экспериментальный, коридор сужен заранее.
  - contracts/info.client-6985.json — чем подтверждается, что convar жив в текущей сборке.
Выход:
  - include/ovr/convars/convar_whitelist.hpp — таблица как данные: {имя, уровень риска, мин, макс,
    дефолт, ссылка на RESEARCH.md:160}; ни одной константы-смещения.
  - src/convars/convars.cpp — apply_from_config(const config::Config&, memory::IMemoryBackend*)
    и restore_all(...): сигнатуры — дословно §10.8 (ARCHITECTURE.md:830), только эти две функции
    видны снаружи; писателя (§10.6) передаёт вызывающий, фича его не видит.
  - src/convars/CMakeLists.txt — цель ovr_convars: ovr_core, ovr_memory, ovr_config.
  - tests/unit/test_convar_whitelist.cpp — единственный тест задачи.
  - src/main.cpp — ровно одна строка `convars::apply_from_config(cfg, backend, writer, offsets);`
    (аргументы берёт из `app::Context`, §10.16); больше правок в этом файле нет.
  - корневой CMakeLists.txt не трогать: guarded add_subdirectory(src/convars) уже создан T000 (P5).
Контракт (из ARCHITECTURE.md:336 и ARCHITECTURE.md:830):
  struct Convar { std::string_view name_or_handle; int min, max, def, risk; };
  class ConvarTable {
  public:
    static ConvarTable built();                 // таблица как данные: RESEARCH.md:160
    std::span<const Convar> all() const;
    const Convar* find(std::string_view) const;  // nullptr = писать нельзя
  };
  bool apply_from_config(const config::Config&, memory::IMemoryBackend&,
                         memory::IMemoryWriter&, const offsets::OffsetRegistry&);
  std::uint32_t restore_all(memory::IMemoryBackend&, memory::IMemoryWriter&);
  Значения берутся ТОЛЬКО из Config (T021); аргумент-константа допустима лишь для r_farz,
  вычисляемого как 2 * `cv.camera_distance` (RESEARCH.md:160).
  Нет имени в OffsetRegistry => не писать и вернуть false с именем ключа (P10), а не подставлять
  «похожий адрес». Счётчик записей возвращается тесту и показывается в меню, чтобы «ничего не
  изменилось» было чем проверить (§10.8).
  Настоящие имена convar-ов, в которых есть имя хоста, берутся из `ovr/host_env.hpp` (P14):
  `cv.camera_distance`, `cv.range_display`, `cv.particle_fow` — это ручки, литералов хоста в
  `src/convars/**` быть не должно (проверяет check_host_env.py из T023).
  Разрешённые имена — построчно из RESEARCH.md:160: fog_enable (0..1, средний риск),
  fow_client_nofiltering (0..1, низкий), `cv.camera_distance` (1200..1500),
  r_farz (производное), `cv.range_display` (низкий), cl_weather (только 0..10).
  Запрещено считать convar-ом: `cv.particle_fow` (удалён в 6985), set_rendering_enabled
  (это метод, не консольная переменная), `sv_cheats` (высокий риск, в стандартном сетевом режиме недоступен).
  apply_from_config идемпотентен: повторный вызов с тем же конфигом не делает ни одной записи.
  restore_all() возвращает дефолты из таблицы и повторный вызов тоже ничего не пишет.
Крайние случаи: E23, E24, E25 (каталог ARCHITECTURE.md:999 §11).
Ограничения:
  Писать можно только через IMemoryBackend (T006/T006W): никаких своих указателей, своих
  перехватов и прямого ReadProcessMemory/WriteProcessMemory внутри src/convars/**.
  Адрес convar-а получается через OffsetRegistry (ARCHITECTURE.md:175); если имени нет в
  реестре -> не писать и вернуть false с текстом «не Established», угадывать адрес запрещено
  (ARCHITECTURE.md:628 (§8.2)).
  Если одна строка в src/main.cpp не помещается (нет места после загрузки конфига) -> ответ
  «контракт неполон» + дифф ровно из одной строки; переписывать main.cpp нельзя.
  Погодного ключа в образце конфига нет (RESEARCH.md:245): cl_weather брать из
  существующего поля конфига либо вернуть «контракт неполон» (владелец ключей T021).
DoD / тест:
  1) Таблица whitelist == RESEARCH.md:160 построчно (имя, диапазон, риск); лишней строки нет,
     запрещённых имён (`cv.particle_fow`, set_rendering_enabled, `sv_cheats`) в коде нет.
  2) cl_weather==11 в обход validate() -> ни одной записи + сообщение с именем ключа.
  3) E24: apply_from_config на fake-бэкенде записал ровно N значений (N посчитано в тесте); повторный
     вызов с тем же конфигом -> 0 записей.
  4) E23: convar-а нет в OffsetRegistry либо он удалён в этой сборке (`cv.particle_fow`) ->
     find() отдаёт nullptr, apply_from_config отвечает false с именем ключа и делает 0 записей;
     удалённый convar в реестре смещений вообще не ищется (ни одного обращения по этому имени);
  5) E25: фича выключена, но конфиг просит значение convar-а -> записей ноль: канал записи
     открывается только явным переключателем конфига (R2), при выключенном счётчик записей 0;
  6) restore_all() -> все значения равны дефолтам таблицы; второй restore_all() -> 0 записей.
  7) grep -rn "WriteProcessMemory\|process_vm_writev\|WriteMemory(" src/convars — только вызовы
     через IMemoryBackend (реальных syscall-имён в src/convars/** быть не должно).
  8) grep -c "convars::apply_from_config" src/main.cpp -> 1; больше diff-строк в main.cpp нет.
  9) python3 scripts/check_includes.py (общее правило 14: до T023 рёбра include проверяются вручную по §4.2):
     src/convars/** не включает render/, overlay/, features/.
  10) cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j &&
     ctest --test-dir build --output-on-failure — зелёные без хоста.
Не трогать: src/features/**, src/input/**, src/render/**, src/overlay/**, include/ovr/config/**, contracts/** (кроме чтения).
```

### T023 — CI: проверка рёбер, литералов смещений и актуальности контрактов

```text
ID: T023
Milestone: M6. Зависимости: T001, T002, T011.
Цель:
  Превратить правила архитектуры и порядок обновления дампа в исполняемые проверки, чтобы
  «модульный монолит» (ARCHITECTURE.md:11) держался без контроля человека: три скрипта + два
  workflow. Правила живут как данные со ссылками на разделы, а не в голове исполнителя.
  Файлы `.github/workflows/**` к этой задаче отношения не имеют: это T032.
Вход:
  - ARCHITECTURE.md:41 (§2) — слои и дерево каталогов: из чего строить список разрешённых рёбер.
  - ARCHITECTURE.md:419 (§4.1) — таблица «директория -> задача»: кто чем владеет.
  - ARCHITECTURE.md:444 (§4.2) — что запрещено пересекать: готовые правила для check_includes.py.
  - ARCHITECTURE.md:175 (§3.3) и ARCHITECTURE.md:584 (§8) — запрет литералов смещений >= 100000.
  - ARCHITECTURE.md:519 (§7) — три законные IPC-границы; их скрипты не считают нарушением.
  - ARCHITECTURE.md:611 (§8.1) — платформы: CI собирается на Linux, Windows-исключения не ломают checks.
  - RESEARCH.md:302 (§11) — порядок обновления по расписанию: version_file -> ClientVersion -> коммит при диффе.
  - RESEARCH.md:322 (§12) — что уже проверено в Tor/GitHub, чтобы скрипты это не переоткрывали.
  - contracts/README.md + scripts/check_contracts.py — валидатор T002, его не дублировать.
Выход:
  - scripts/check_includes.py — правила рёбер как таблица данных (какой glob что имеет право
    include-ить); нарушение печатает path:line и exit 1.
  - scripts/check_no_hardcoded_offsets.py — литералы >= 100000 в src/** и include/**; build/ и
    tests/** вне проверки; находка -> path:line и exit 1.
  - scripts/redump_offsets.py — читает version_file (ручка, имя — в `HOST-ENV.json`), сравнивает ClientVersion с
    contracts/info.client-6985.json; diff пуст -> «no change»; иначе печатает список изменений.
  - scripts/check_host_env.py — стоп-лист строится из значений HOST-ENV (`host.name`, `handles`,
    имена `sources`): ищет настоящие имена в файлах репозитория и в сообщениях коммитов
    (`git log --format=%s`); находка -> path:line и exit 1; словарь семьи движка
    (`engine_vocabulary`) нарушением не считается; аргумент `--env <файл>`, `--selftest`
  - scripts/check_docs.py — перенос существующей проверки связности документов как есть
    (в текущем виде она живёт в репозитории заметок; в CI это шаг T032).
  - tests/fixtures/ci/** — фикстуры для --selftest (сломанный include, лишняя константа,
    изменённый version_file), пишутся в build/selftest/.
Контракт (из ARCHITECTURE.md:444):
  Каждый скрипт: exit 0 если нарушений нет, exit 1 если есть; вывод по одной строке на нарушение
  в формате path:line: сообщение; на stderr — ничего постороннего.
  У каждого скрипта флаг --selftest: офлайн, сети не трогает, сам делает фикстуры в
  build/selftest/ и проверяет, что нарушение обнаруживается (exit 1) и не обнаруживается (exit 0).
  redump_offsets.py никогда не правит contracts/** сам: он либо говорит «no change», либо
  exit 0 + список ключей, которые должен пересобрать T011; без хоста на машине -> не падать,
  а напечатать not-established и exit 0 (ARCHITECTURE.md:628 (§8.2)).
Крайние случаи: E1, E30, E41, E42 (каталог ARCHITECTURE.md:999 §11).
Ограничения:
  Python 3 и только стандартная библиотека: модуля yaml нет, YAML-проверки делать построчными
  регулярками; новых зависимостей и `pip install` ни у одного скрипта быть не должно.
  `.github/**` этой задачей не создаётся: шаг, который никто не запускает, проверкой не является.
  scripts/check_contracts.py и contracts/schemas/** — артефакты T002: не пересоздавать, не
  переписывать, можно только вызывать; в contracts/** пишет только T011 (§4.2:404).
  dwBuildNumber в сборке 6985 удалён (RESEARCH.md:302): сверять только ClientVersion.
  Коммит только при непустом диффе; git-коммиты делает CI, не исполнитель задачи.
  src/** и include/** не править: если правило §4.2 не проверить скриптом -> «контракт неполон».
DoD / тест:
  1) python3 scripts/check_includes.py --selftest -> exit 0; с фикстурой-нарушением -> exit 1 и
     в выводе путь:строка этого include.
  2) python3 scripts/check_no_hardcoded_offsets.py --selftest -> константа 698512 в src/ даёт
     exit 1, константа 99999 даёт exit 0; файл в build/ или tests/ не проверяется.
  3) E30: python3 scripts/redump_offsets.py --selftest --offline (фикстура version_file с другим
     `ClientVersion` — это и есть «пришёл патч»): дифф пуст -> «no change» exit 0; дифф непуст ->
     exit 0 и построчный список изменённых ключей, а contracts/** и src/** не тронуты ни на строку
     (новый патч = новый JSON, P3).
  4) E41: все скрипты на текущем дереве -> exit 0 (redump при отсутствии хоста -> not-established);
     ни один скрипт не требует хоста, GPU, монитора или сети.
  5) E1: среды хоста нет вовсе (ни файла HOST-ENV, ни `OVR_HOST_ENV`): check_host_env.py и
     check_docs.py дают exit 0 на фикстуре tests/fixtures/host_env.sample.json,
     gen_host_env.py печатает not-established и exit 0, а не выдуманные имена (P10);
  6) E42: python3 scripts/check_host_env.py --selftest -> фикстура с настоящим именем хоста в `src/`
     даёт exit 1 и `path:line`; имя из `engine_vocabulary` (например `sv_cheats`) -> exit 0.
  7) grep -rn "import yaml\|pip install\|requests\." scripts — пусто (зависимостей нет).
  8) python3 scripts/check_docs.py -> exit 0 на текущем дереве (он же шаг CI у T032);
     python3 scripts/check_contracts.py contracts/*.json -> exit 0, и scripts/check_contracts.py
     не изменён (это файл T002).
  9) cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j &&
     ctest --test-dir build --output-on-failure — зелёные без хоста.
Не трогать: src/**, include/**, scripts/check_contracts.py, contracts/schemas/**, contracts/**
(только чтение), .github/** (это T032).
```

---

### T032 — CI: workflow-файлы и шаги поверх скриптов T023

```text
ID: T032
Milestone: M6. Зависимости: T002, T011, T023.
Цель:
  Превратить скрипты T023 в конвейер, который реально выполняется на пуш: две платформы,
  кеш сборки, артефакт с тестами. Скрипты остаются в `scripts/` (владелец T023), здесь только
  `.github/workflows/**` — иначе проверка «есть» и проверка «работает» разъезжаются.

Вход:
  - scripts/check_docs.py, scripts/check_host_env.py, scripts/check_includes.py,
    scripts/check_no_hardcoded_offsets.py, scripts/check_contracts.py, scripts/redump_offsets.py
    — готовые шаги из T023 (запускать как есть, не переписывать).
  - ARCHITECTURE.md:584 (§8) — приёмка на Linux; ARCHITECTURE.md:611 (§8.1) — запрет настоящих имён.
  - ARCHITECTURE.md:999 (§11) — E28, E41, E42.
  - AGENTS.md — правило «git не делать»: workflow коммитит заказчик, исполнитель только готовит файл.

Выход:
  - .github/workflows/ci.yml — (linux + windows: configure, build, ctest, все scripts/*,
    артефакт с логом тестов; без секретов и без сети кроме репозитория)
  - .github/workflows/contract-freshness.yml — (ежедневный `redump_offsets.py --dry-run`:
    без хоста он печатает not-established и exit 0, E41)
  - .github/workflows/README.md — (что падает и как это починить локально одной командой)

Контракт:
  - Шаги ci.yml по порядку: `cmake -S . -B build`, `cmake --build build -j`,
    `ctest --test-dir build --output-on-failure`, затем каждый скрипт из `scripts/` отдельным
    шагом (падать должен конкретный шаг, а не «CI красный»).
  - Ни один шаг не требует хоста, GPU, монитора и интернета, кроме `actions/checkout` и
    установки тулчейина (E28).
  - Настоящие имена хоста проверяются `check_host_env.py` по дереву И по истории коммитов (P14).
  - Файлы workflow не содержат секретов: если шаг требует хоста, он `if: false` с комментарием,
    а не выдуманный зелёный запуск.

Крайние случаи: E28, E41, E42 (каталог ARCHITECTURE.md:999 §11).
Ограничения:
  - Эта задача не трогает `scripts/**` и не меняет CMake: только `.github/**`.
  - Никаких self-hosted раннеров и «прогревов» ценой секретов.
  - Windows-джоб строится тем же CMake-файлом, что и Linux: отдельных флагов платформы в workflow
    быть не должно (P7).
DoD / тест:
  1) `python3 -c "import yaml,sys;yaml.safe_load(open('.github/workflows/ci.yml'))"` без ошибок
     (если pyyaml нет — проверка `headless`-разбором в самом workflow README);
  2) E28: каждый шаг ci.yml повторяем локально одной командой из `.github/workflows/README.md`,
     и все команды Finish successfully на Linux без хоста, без GPU и без монитора (ни один шаг
     окна не создаёт);
  3) E42: добавить в ветке файл с настоящим именем хоста -> шаг `check_host_env.py` единственные
     красные, и в логе видно `path:line`;
  4) E41: `redump_offsets.py` в workflow без хоста даёт not-established и exit 0, job зелёный;
  5) cron: `grep -n "cron" .github/workflows/contract-freshness.yml` — ровно "0 17 * * *"
      (RESEARCH.md:302); в `ci.yml` cron нет вовсе;
  6) в `.github/**` нет настоящих имён хоста и нет секретов (`grep -rn "secrets." .github` -> пусто);
  7) `ctest --test-dir build --output-on-failure` — зелёный набор, к workflow не при чём.
Не трогать: scripts/** (это T023), CMakeLists.txt, src/**, include/**, contracts/**.
```
---

## Таблица зависимостей

Столбец «Блокирует» — только прямые зависимые (обратные рёбра к полю `Зависимости:` в блоках;
транзитивных замыканий в таблице нет). «->» читается как «потому что».

| ID | MS | Зависит от | Блокирует (напрямую) | Почему |
|---|---|---|---|---|
| T000 | M0 | — | T001, T002 | Дерево каталогов + CMake с зелёной сборкой: без этого нечего тестировать и некуда линковать цели. |
| T001 | M0 | T000 | 15: T003-T006, T006W, T007, T010, T012-T015, T019, T021-T023 | `core/` — единые типы, математика и монотонное время; иначе каждая задача выдумает свой vec3 и свои часы. |
| T002 | M0 | T000 | T003, T008, T021, T023, T032 | JSON-схемы + `check_contracts.py`: контракты проверяются машинно, а не «на глаз»; валидатор нужен и дампу, и схеме конфига, и CI. |
| T003 | M1 | T001, T002 | T006, T008, T010, T011 | OffsetRegistry держит смещения как данные (ARCHITECTURE.md:175) и включает проверку устаревания сборки. |
| T004 | M0 | T001 | T005, T006, T006W, T019, T022 | Один заголовок `IMemoryBackend` — шов между «реальный процесс» и «файл с дампом»; на нём держатся все модульный тесты M3+. |
| T005 | M0 | T001, T004 | T006, T006W, T015 | `FakeBackend`: без него ни один парсер нельзя проверить без запущенного хоста, GPU и монитора. |
| T006 | M2 | T001, T003, T004, T005 | T006W, T007-T009, T022 | Чтение живого процесса на Windows (`ReadProcessMemory`): точка, где фейка уже не хватает. |
| T006W | M2 | T001, T004, T005, T006 | T015 | Linux-бэкенд `process_vm_readv` повторяет контракт T006, поэтому идёт после него, а не параллельно: иначе разъедутся сигнатуры. |
| T007 | M3 | T001, T006, T021 | 16: T008, T009, T015-T018, T020, T024-T031, T033 | Публичные `HostState`/`Agent` — единственный источник данных для фич; поменяешь типы -> пересобирается весь M5. |
| T008 | M3 | T002, T003, T006, T007, T011 | T009, T015, T033 | Обход entity list и сетевых слотов (RESEARCH.md:127): самая объёмная часть парсера, невозможна без дампа полей. |
| T009 | M3 | T006, T007, T008, T011 | — | Golden-снапшоты закрывают M3 и становятся регресс-гарантом для любой будущей правки парсера. |
| T010 | M1 | T001, T003 | T011 | Скан паттернов по образу процесса: дамперу нужно находить то, чего нет в JSON-контракте (RESEARCH.md:183). |
| T011 | M1 | T003, T010 | 15: T008, T009, T013, T017, T018, T023-T032 | Критический путь всего проекта: без `m_*`- и view-полей (Q1/Q2) метки, проекция мир->экран, каркас и тайминги воздействий физически невозможны (RESEARCH.md:199). |
| T012 | M4 | T001, T015 | 15: T013, T014, T016-T018, T020, T024-T031, T033 | `FrameDraw` — общий контракт «фича рисует примитивами и не аллоцирует в run()»; размер буфера берётся из замера T015. |
| T013 | M4 | T001, T011, T012 | 13: T014, T016-T018, T024-T031, T033 | `world_to_screen`: без проекции примитив не знает, куда его рисовать; сама проекция чистая, но требует view-полей T011 (RESEARCH.md:214). |
| T014 | M4 | T001, T012, T013 | T016, T033 | Одно окно оверлея + меню по хоткею: без него кадры негде показать, а настройками нельзя управлять вообще. |
| T015 | M3 | T001, T005, T006W, T007, T008, T021 | T012, T019, T033 | Бенчмарк бюджета кадра + SHM-снапшоты: даёт T012 число, под которое проектируется буфер (ARCHITECTURE.md:456). |
| T016 | M4 | T007, T012, T013, T014, T021 | 12: T017, T018, T020, T024-T031, T033 | `FeatureRegistry` и `feature.hpp` (ARCHITECTURE.md:363): реестр нужен всем фичам M5, поэтому M5 стартует только после T016; метки акторов вынесены в T031. |
| T017 | M5 | T007, T011, T012, T013, T016, T021, T033 | — | Индикатор добивания (`last-hit`/`deny`, RESEARCH.md:231): пороги урона — данные; только чтение, рисует, ничего не пишет. |
| T018 | M5 | T007, T011, T012, T013, T016, T021, T033 | — | Таймеры меток/стоянок/скоплений (RESEARCH.md:232): самый ранний победитель — `dwGlobalVars` плюс типы сущностей. |
| T019 | M6 | T001, T004, T015, T021 | T020 | Input-бэкенды: единственная легальная точка отправки клавиш, запись в `CInput` потребовала бы перехвата (RESEARCH.md:346, Q8). |
| T020 | M6 | T007, T012, T016, T019, T021, T022, T033 | — | «Связка» (RESEARCH.md:234) — единственная input-фича класса I, по умолчанию выключена: её законность в стандартном сетевом режиме не подтверждена (Q8). |
| T021 | M2 | T001, T002 | 17: T007, T015-T020, T022, T024-T031, T033 | Конфиг — единственный источник настроек (RESEARCH.md:245): до него любая фича захардкодила бы цвет, порог и задержку. |
| T022 | M6 | T001, T004, T006, T021 | T020 | Convar-ы: вся запись в хост в одном месте и только по белому списку (RESEARCH.md:160); до него W-фичи пишут «кто куда». |
| T023 | M6 | T001, T002, T011 | T032 | CI-проверки рёбер (§4.2), литералов смещений и свежести дампа: поддерживают T003 и T011, сами ничего не блокируют. |
| T024 | M5 | T007, T011, T012, T013, T016, T021, T033 | — | Каркас по парам костей (RESEARCH.md:226): пока Q1 открыта, рисует not-established, а не угаданный каркас. |
| T025 | M5 | T007, T011, T012, T013, T016, T021, T031, T033 | — | Snap lines (RESEARCH.md:227): видимость = рейка по данным сетки видимости, отсюда зависимость от T011 и T013. |
| T026 | M5 | T007, T011, T012, T013, T016, T021, T031, T033 | — | Fog overlay «кто кого видит» (RESEARCH.md:228): только чтение, но нужны поля сетки видимости из дампа. |
| T027 | M5 | T007, T011, T012, T013, T016, T021, T033 | — | Range displays (RESEARCH.md:229): R-версия рисуется сама, W-версия через `cv.range_display` фактически ждёт T022. |
| T028 | M5 | T007, T011, T012, T013, T016, T021, T033 | — | Cooldown tracker (RESEARCH.md:230): фича держится на цепочке полей воздействий из дампа, то есть транзитивно на T011. |
| T029 | M5 | T007, T011, T012, T013, T016, T021, T031, T033 | — | Обманки/истинный обзор (RESEARCH.md:233): `m_hPlayer`-деривация плюс модификаторы — тоже поля из T011. |
| T030 | M5 | T007, T011, T012, T013, T016, T021, T033 | — | Алерты и W-политики (RESEARCH.md:236-235): камера и погода пишут convar-ы, значит полезная часть упирается в T022. |
| T031 | M5 | T007, T011, T012, T013, T016, T021, T033 | T025, T026, T029 | Метки акторов и `IVisionModel` (§10.10): единственное место, где живёт гипотеза о сетке видимости; от неё зависят «кто кого видит» в T025, T026, T029. |
| T032 | M6 | T002, T011, T023 | — | Workflow-файлы поверх скриптов T023: скрипт, который никто не запускает, проверкой не является (E41, E42). |
| T033 | M4 | T007, T008, T012, T013, T014, T015, T016, T021 | 11: T017, T018, T020, T024-T031 | Цикл кадров и CLI `ovr` (§10.12): без `--once N` приёмка не доказывает, что наблюдатель вообще запускается; крюк `register_features` нужен всем фичам M5. |

### Как это запускать

- **Волны (уровень = готовность: всё слева готово; задачи одной строки идут параллельно,
  разным исполнителям):**
  уровень 0 — T000; 1 — T001, T002; 2 — T003, T004, T021; 3 — T005, T010; 4 — T006, T011;
  5 — T006W, T007, T022, T023; 6 — T008, T032; 7 — T009, T015; 8 — T012, T019; 9 — T013;
  10 — T014; 11 — T016; 12 — T033; 13 — T017, T018, T020, T024, T027, T028, T030, T031;
  14 — T025, T026, T029. Волны выводятся из поля `Зависимости:`, колонка «Блокирует» — те же
  рёбра в обратную сторону: руками их не правят, иначе таблица и блоки разъедутся (P11).
- **Критический путь по зависимостям (15 шагов):** T000 -> T001 -> T004 -> T005 -> T006 -> T007
  -> T008 -> T015 -> T012 -> T013 -> T014 -> T016 -> T033 -> T031 -> T025/T026/T029. Точка,
  где всё сходится, — T033: до цикла кадров ни одна фича не проверяется как часть наблюдателя.
- **Критический путь по времени — T011:** 15 прямых зависимых, и это единственная задача,
  которой нужен живой хост и человек с клиентом. Начинавать её надо в первый же день: волна 4
  по зависимостям не означает, что её можно откладывать. Пока Q1/Q2 (RESEARCH.md:335) не
  закрыты, M4/M5 рисуют not-established, но не блокируются сборкой.
- **Один исполнитель = одна задача:** параллельно можно запускать только задачи из одной волны,
  у которых не пересекаются поля `Выход:` (ARCHITECTURE.md:419); единственный общий файл —
  `src/main.cpp` с крюками по одной строке (см. «Общие правила» в начале документа).
- **Осознанная инверсия порядка:** T027 и T030 заявлены как read-only (M5), а их W-варианты
  включают только после T022 (M6). Если нужен жёсткий порядок — добавить T022 в `Зависимости:`
  T027 и T030; делать это в уже принятой задаче нельзя, это правка контракта.
- **Вне таблицы:** наводчик (RESEARCH.md:241) — в бэклоге, ID не назначен. Если делать, он
  встаёт волной после T019 и T022, потому что ему нужны и ввод, и whitelist.
- **Арбитр противоречий:** `PRINCIPLES.md` (P1–P14). Решения P4 (арена вместо `std::vector`),
  P5 (guarded `add_subdirectory` из T000), P6 (ключи фич объявляет T021), P7 (Linux-бэкенд в своей
  директории), P8 (golden-фикстуры с `MANIFEST.sha256`) и P14 (настоящие имена хоста только через
  ручки) уже внесены в блоки; волн это не меняет.

# TASKS — 32 задачи, каждая самостоятельна

Документ разбит так, чтобы одну задачу можно было скопировать целиком в чистую worktree
сильной модели, у которой **нет знания хоста и нет знания остальных задач**. Всё нужное
задача говорит прочитать; всё, что задача обязана вернуть, — зафиксировано сигнатурами.

## Как делегировать

- **Один fenced-блок = одна worktree = один исполнитель.** Никогда не отдавай две задачи
  одному исполнителю: они могут пересечься файлами (см. `ARCHITECTURE.md:379`).
- Порядок задаётся milestone'ами ниже и таблицей зависимостей в конце. Задачи внутри
  одного milestone'а без пересечения зависимостей можно выполнять параллельно.
- Исполнитель получает: текст задачи + право читать файлы из поля «Вход» + свою worktree.
  Он **не получает** ни `RESEARCH.md`, ни `ARCHITECTURE.md` целиком — только указанные строки.
- Приёмка любой задачи: `cmake --build build && ctest --test-dir build --output-on-failure`
  на Linux (`ARCHITECTURE.md:544`) + 5 пунктов DoD (`ARCHITECTURE.md:588`).
- Если контракта не хватает — исполнитель **не додумывает**, а возвращает «контракт неполон»
  и предлагает минимальную правку заголовка комментарием в PR. Правку делает ревьюер.

## Общие правила (действуют на все задачи, в блоках не повторяются)

- Язык C++20, стиль как в уже существующем коде репозитория, comments минимальны.
- Критерии и спорные решения — `PRINCIPLES.md` (P1–P11). Если блок задачи им противоречит,
  отвечает `PRINCIPLES.md`, а исполнитель возвращает «контракт неполон», а не додумывает.
- **Никаких перехватов, правок чужого кода, подсадки модулей** — наблюдатель внешний (`ARCHITECTURE.md:461` R1).
- **Никаких литералов смещений `>= 100000`** в `src/` и `include/` — только
  `OffsetRegistry::lookup()` (`ARCHITECTURE.md:146`). Проверка: `scripts/check_no_hardcoded_offsets.py`.
- **Никаких процессов кроме одного.** Никаких сокетов между модулями (`ARCHITECTURE.md:11`).
- Никаких `TODO`/`FIXME` вместо реализации. Неизвестное поле = значение помечается
  `not-established` в описании результата, а не захардкожено.
- Тестам запрещено: спать, читать `/proc`/реестр, зависеть от порядка запуска, требовать
  GPU/монитора/интернета, оставлять файлы вне `build/` (`ARCHITECTURE.md:544`).
- `git init`/`git commit`/`git push` в этой работе не делать: коммиты остаются за заказчиком.
- Единственный разрешённый общий файл — `src/main.cpp`: два крюка по одной строке —
  `registry.add(...)` в `register_features()` — это все фичи — и `convars::apply_from_config`
  (`cfg, backend`, владелец — T022). «Ровно одна строка» значит «на одну больше, чем было
  до начала задачи»: сколько фич смержено, столько таких строк в файле. Конфликт при merge
  разрешает тот, кто мержит позже; больше в `main.cpp` менять ничего нельзя.
- Терминология: тексты, идентификаторы, строки вывода, имена файлов и ключи конфига — только
  слова из `TERMS.md`; внешние названия (репозитории, API, convar-ы) — в `бэктиках` (P12).

---

# M0 — каркас, на котором всё можно собирать

### T000 — скелет дерева, CMake, зелёная сборка

```text
ID: T000
Milestone: M0. Зависимости: нет.
Цель: создать пустой, но собирающийся каркас проекта, чтобы каждая следующая задача
      клала файлы в уже существующие директории и сразу попадала в ctest.

Вход (прочитать обязательно):
  - ARCHITECTURE.md:41   — дерево каталогов и правило «включать только соседний снизу слой»
  - ARCHITECTURE.md:571  — платформы, зависимости, команда Linux-сборки
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

DoD / тест:
  1) mkdir build && cmake -S . -B build -DOVR_BUILD_TESTS=ON && cmake --build build
  2) ctest --test-dir build --output-on-failure  → зелёно
  3) те же команды с -DOVR_BUILD_TESTS=OFF       → сборка без тестов тоже зелёная
  4) `tree -L 2 include src tests` совпадает с ARCHITECTURE.md:41 (с учётом .gitkeep)
  5) python3 scripts/gen_host_env.py --env tests/fixtures/host_env.sample.json ->
     в заголовке значения фикстуры (`sample.bin`), сборка зелёная
  6) тот же скрипт без `--env` и без `OVR_HOST_ENV` -> `is_available() == false`, пустые
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
  - ARCHITECTURE.md:41    — правило include-графов

Выход:
  - include/ovr/core/types.hpp
  - include/ovr/core/vec.hpp
  - include/ovr/core/time.hpp
  - src/core/vec.cpp
  - tests/unit/test_core.cpp

Контракт (из ARCHITECTURE.md:114):
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

Ограничения: только <cstdint>, <array>, <cmath>, <chrono>. Никаких <windows.h>, <string> в types.hpp.
DoD / тест (tests/unit/test_core.cpp, без хоста и без файлов):
  - Mat4x4{} == все нули; toждественная матрица задаётся явным конструктором identity()
  - transform_point(identity, {1,2,3}) == {1,2,3}; transform_point с w<=0 возвращает {0,0,0} и не падает
  - operator* Mat4x4: M*I == M, ассоциативность на 3 матрицах с допуском 1e-5
  - normalize({0,0,0}) не даёт NaN
  - static_assert(sizeof(Mat4x4) == 64)
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

DoD / тест:
  - python3 scripts/check_contracts.py contracts/offsets.client-6985.json contracts/schemas/offsets.schema.json → exit 0
  - python3 scripts/check_contracts.py <каждый invalid/*> → exit 1 (перебрать в shell-цикле)
  - python3 scripts/check_contracts.py diff contracts/offsets.client-6985.json tests/fixtures/contracts/valid/offsets.client-6985.json → diff пустой, exit 0
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
  - ARCHITECTURE.md:126  — §3.2, точная сигнатура
  - RESEARCH.md:283      — §10: Windows RPM vs Linux process_vm_readv
  - ARCHITECTURE.md:416  — §5: почему батч, а не 400 отдельных read()

Выход:
  - include/ovr/memory/memory_backend.hpp    (header-only интерфейс; .cpp нет)

Контракт (из ARCHITECTURE.md:126):
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
  Комментарий в заголовке обязан фиксировать семантику: read_batch НЕ бросает исключений,
  при провале чтения dst остаётся нетронутым; module_base возвращает 0, если модуль не найден.

Ограничения: в заголовке разрешены <cstdint>, <string_view>, <span>. Никаких windows.h / sys/uio.h.
DoD / тест: заголовок компилируется сам по себе (TU, включающий его дважды через pragma + include);
  ни одна из строк `#include <windows.h>`/`<sys/uio.h>`/`<dlfcn.h>` в файле отсутствует
  (проверяется grep в DoD-скрипте задачи).
Не трогать: src/memory/** — реализаций здесь не делать, это T005/T006/T006W.
```

### T005 — `FakeBackend`: память из файла-дампа

```text
ID: T005
Milestone: M0. Зависимости: T001, T004.
Цель: читаемая «память хоста» из фикстур, чтобы 90% тестов M2+ проходили на машине БЕЗ хоста
      и без Windows. Fake обязан появиться раньше живого бэкенда.

Вход:
  - ARCHITECTURE.md:126  — §3.2 (реализуемый интерфейс)
  - ARCHITECTURE.md:544  — §8: Fake раньше живого, тесты без хоста
  - ARCHITECTURE.md:416  — §5: одна пачка read_batch() за кадр

Выход:
  - src/memory/fake_backend/fake_backend.hpp
  - src/memory/fake_backend/fake_backend.cpp
  - src/memory/fake_backend/dump_loader.hpp, dump_loader.cpp
  - tests/fixtures/dumps/minimal.bin, tests/fixtures/dumps/minimal.map.json
  - tests/unit/test_fake_backend.cpp

Контракт:
  - class FakeBackend final : public memory::IMemoryBackend
  - строится из dump-файла: load(path) → адресная карта [{base, module, bytes}]
    minimal.map.json: [{ "module": "client.dll", "base": "0x180000000", "file": "minimal.bin", "file_offset": 0, "size": 4096 }, ...]
  - attach(pid) игнорирует pid и возвращает true, если load прошёл
  - module_base("client.dll") == base из карты; неизвестный модуль → 0
  - read_raw: полное попадание в карту → ok=true; выход за границу → ok=false и dst НЕ трогается
  - read_batch: семантически == N вызовов read_raw, но internal counter batch_calls++
    (тест проверяет batch_calls == 1 на пачке из 400 запросов)
  - тестовая подмена set_fail_range(addr, size): чтения внутри диапазона проваливаются (для тестов валидности)

DoD / тест: 500 разных read_raw дают тот же результат, что и один read_batch из 500 ReadReq;
  чтение за концом карты → ok=false, буфер не изменён (заполнить 0xAA и сравнить);
  module_base("inputsystem.dll") и module_base("nope.dll") → 0 и !=0 по карте;
  set_fail_range → ok=false без исключений; ни одного вызова open() внутри read_batch.
Не трогать: src/memory/rpm_backend/** (это T006), include/ovr/memory/memory_backend.hpp.
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
  - ARCHITECTURE.md:146 — раздел 3.3, точные сигнатуры (цитата ниже), читать как данность.
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
Контракт (из ARCHITECTURE.md:146):
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
Ограничения:
  Никаких литералов `>= 100000` в src/ и include/ (сами числа живут только в JSON); в тестах
  числа писать можно. Никаких std::cerr кроме одной строки в load_from. Никакого кэша на диске.
DoD / тест:
  1) lookup("client.dll!dwGameEntitySystem") → rva==108484944, deref==Once;
  2) lookup("client.dll!dwGameEntitySystem_entityCount") → rva==8384, deref==None;
  3) lookup("engine2.dll!dwNetworkGameClient_deltaTick") → rva==588;
  4) lookup("client.dll!dwViewMatrix_static") → nullopt (в 6985 удалён, RESEARCH.md:27);
  5) valid.json + build 6985 → is_stale(6985)==false, is_stale(6986)==true, is_stale(0)==false;
  6) stale.json + build 6985 → пустой реестр; malformed.json и empty.json → пустой реестр,
     без исключений (обернуть в try/catch и проверить, что catch не сработал);
  7) sha256() совпадает с `sha256sum contracts/offsets.client-6985.json | cut -c1-64`;
  8) `python3 scripts/check_contracts.py tests/fixtures/offsets/valid.json` — exit 0;
  9) `cmake --build build && ctest --test-dir build -R offset_registry --output-on-failure`.
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
  - ARCHITECTURE.md:146 — формат Symbol/Deref (цель скана в итоге отдаётся в том же виде).
  - RESEARCH.md:100 — методология: поле принимается только при >=2 независимых источниках.
  - ARCHITECTURE.md:461 — риск R10 (тот же критерий, формулировка для комментариев).
  - include/ovr/offsets/offset_registry.hpp — заголовок из T003 (читать как данность).
  - RESEARCH.md:72 — где берутся дампы образов и как устроены IDA-подобные записи.
Выход:
  - include/ovr/offsets/sigscan/sigscan.hpp — (подректорий, чтобы не пересекаться с T003)
  - src/offsets/sigscan/sigscan.cpp — (разбор шаблона и поиск)
  - src/offsets/sigscan/CMakeLists.txt — (цель ovr_sigscan, линкуется только к ovr_core)
  - tests/unit/test_sigscan.cpp — (синтетические байты, без хоста и без диска)
Контракт (из ARCHITECTURE.md:146):
  Сканера в §3 нет — заголовок фиксируется этой задачей в форме ниже, ревьюер переносит
  его в ARCHITECTURE.md 3.3 комментарием. Форма:
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
Ограничения:
  Вызывать scan() разрешено только из tools/dumper/** (T011); в src/state/**, src/features/**,
  src/render/** упоминаний sigscan быть не должно. Скан linear + BM-подобный skip по первому
  фиксированному байту, никаких внешних библиотек.
DoD / тест:
  1) образ 4 КБ с вписанным «48 8B 05 3A 12 00 00» → scan() даёт ровно 1 Match с ожидаемым адресом;
  2) шаблон с `??` в середине находит оба различающихся по этому байту вхождения (2 Match);
  3) шаблон «48 ?? 8B» находит по старшему/младшему нибблу по правилу `?X`;
  4) rip_target() на известном шаблоне даёт ожидаемый адрес; disp_offset за концом образа → nullopt;
  5) пустой image, пустой шаблон, мусор «zz zz» → пустой результат, без исключений;
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
  - ARCHITECTURE.md:146 — во что должны превратиться добытые смещения (Symbol/Deref/stride).
  - ARCHITECTURE.md:165 — как эти смещения будет есть TypedReader (Once vs Chain, stride).
  - RESEARCH.md:183 — список полей, которые в 6985 НЕ установлены (это и есть цель задачи).
  - RESEARCH.md:100 — критерий принятия поля: >=2 независимых источника или собственный дамп.
  - RESEARCH.md:127 — устройство entity list и сетевых слотов (шаг 24 байта, highest index).
  - contracts/README.md:16 — формат контракта; contracts/README.md:37 — алиасы и версии.
  - contracts/offsets.client-6985.json, contracts/info.client-6985.json — существующие данные.
  - include/ovr/offsets/sigscan/sigscan.hpp — заголовок из T010 (использовать, не переписывать).
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
Контракт (из ARCHITECTURE.md:146):
  Формат contracts/fields.client-6985.json — как у offsets.client-6985.json (ARCHITECTURE.md:146),
  но каждый элемент обязателен с полями rva, deref, stride (0 если не Chain) и evidence: массивом
  из >=2 строк {"source": "...", "note": "..."} по RESEARCH.md:100. Поля, которые добыть не удалось,
  записываются с rva==0, deref=="None" и evidence:[{"source":"not-established","note":"<почему>"}] —
  это валидный результат, а не ошибка. Требуемый минимум ключей:
    client.dll!m_hPlayer, m_iTeamNum, m_iHealth, m_iMaxHealth, m_vecAbsOrigin,
    m_hUnit, m_iLevel, m_lifeState, m_bStunned (актор/агент);
    client.dll!dwViewRender -> view_matrix, view_fov, view_znear, view_zfar (цепочка, stride).
Ограничения:
  T011 — ЕДИНСТВЕННАЯ задача, которой разрешено писать в contracts/** (ARCHITECTURE.md:404);
  правки RESEARCH.md и ARCHITECTURE.md делает ревьюер, не исполнитель. Дампер живёт только в
  tools/** и не линкуется в читаемый бинарник. Запуск с живым хостом допустим ТОЛЬКО в дампере и
  в ad-hoc прогоне, в ctest его быть не должно. Если живого хоста в окружении нет — не выдумывать:
  вернуть контракт с not-established и список того, что нужно добыть руками.
DoD / тест:
  1) `python3 scripts/check_contracts.py contracts/fields.client-6985.json` → exit 0;
  2) `grep -c not-established contracts/fields.client-6985.json` посчитан и объяснён в tools/dumper/README.md;
  3) OffsetRegistry из T003, расширенный этим файлом, возвращает lookup("client.dll!m_vecAbsOrigin")
     с ненулевым rva (или явно not-established — но тогда это зафиксировано в DoD-отчёте);
  4) `ctest --test-dir build --output-on-failure` зелёные БЕЗ запуска хоста (дампер в тестах не запускается);
  5) `cmake --build build` собирает ovr_dumper; `./build/tools/dumper/ovr_dumper --help` печатает usage;
  6) ни один файл за пределами «Выход» не изменён (проверить `git status`-эквивалентом: спискомdiff).
  7) contracts/MANIFEST.sha256 покрывает contracts/fields.client-6985.json, sha256sum сходится (P8).
Не трогать: RESEARCH.md, ARCHITECTURE.md, contracts/offsets.client-6985.json (читать), src/**, include/**.
```

---

# M2 — Чтение памяти, конфиг, бюджет кадра

Здесь наблюдатель впервые начинает укладываться в 1.8 мс на кадр (`ARCHITECTURE.md:416`) и перестаёт
требовать перекомпиляции ради настройки. `T015` — единственная задача с числовым DoD: если
бюджет не выдержан, дальше идти нельзя, потому что все фичи M5–M6 добавляют время, а не убирают.

### T006 — TypedReader + бэкенд Windows (ReadProcessMemory)

```text
ID: T006
Milestone: M2. Зависимости: T001, T004, T005, T003.
Цель:
  Научиться читать память хоста типизированно и пачкой: вызов идёт один на всю пачку, а
  разыменование цепочек делает сам читатель, возвращая флаг валидности вместо исключения.
Вход:
  - ARCHITECTURE.md:165 — раздел 3.4, точные сигнатуры (цитата ниже).
  - ARCHITECTURE.md:114 — 3.1 IProcessMemory: контракт read_batch(), который задал T004.
  - include/ovr/memory/memory_backend.hpp — заголовок T004 (читать как данность).
  - include/ovr/offsets/offset_registry.hpp + ARCHITECTURE.md:146 — Symbol/Deref/Chain, stride.
  - src/memory/fake_backend/** — поведение-эталон из T005; живой бэкенд обязан дать те же тесты.
  - RESEARCH.md:283 — почему пакетное чтение обязательно (стоимость одного syscall).
  - ARCHITECTURE.md:416 — §5: один read_batch() на кадр, <= 0.6 мс.
Выход:
  - include/ovr/memory/typed_reader.hpp — (заголовок ровно по 3.4)
  - src/memory/typed_reader.cpp — (разбор цепочек Once/Chain поверх read_batch)
  - src/memory/rpm_backend/read_process_memory_backend.cpp — (класс RpmBackend, IProcessMemory)
  - src/memory/rpm_backend/CMakeLists.txt — (условная компиляция: WIN32 только)
  - tests/unit/test_typed_reader.cpp — (тесты поверх FakeBackend из T005)
  - tests/unit/test_rpm_backend.cpp — (бэкенд-контракт; на Linux skips по платформе)
Контракт (из ARCHITECTURE.md:165):
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
  RpmBackend реализует IProcessMemory из ARCHITECTURE.md:114 ровно в одном методе read_batch():
  одна пачка iovec-подобных запросов -> ОДИН вызов ReadProcessMemory на непрерывный кусок,
  провал одного диапазона не должен ронять всю пачку (частичный ok по флагам).
Ограничения:
  В рантайме не вызывать ReadProcessMemory по одному слову (400 вызовов = 3-5 мс потерь,
  RESEARCH.md:283). Никакого WriteProcessMemory, VirtualAllocEx, CreateRemoteThread, LoadLibrary
  в src/memory/**: запись и подсадка модулей запрещены (RESEARCH.md:47, R1). Типичная пачка <= 256 диапазонов.
DoD / тест:
  1) на FakeBackend: чтение m_hPlayer-подобной цепочки Once->Once->None даёт ожидаемое значение;
  2) цепочка с нулём на втором шаге => ok==false и value==T{} (заполнить value до 0x5A и проверить обнуление);
  3) чтение за концом карты (FakeBackend set_fail_range) => ok==false, без исключений;
  4) unknown root_symbol => ok==false;
  5) `grep -c ReadProcessMemory src/memory/rpm_backend/read_process_memory_backend.cpp` == 1;
  6) `grep -rn "WriteProcessMemory\|VirtualAllocEx\|CreateRemoteThread" src/memory` → пусто;
  7) `cmake --build build && ctest --test-dir build -R "typed_reader|rpm_backend" --output-on-failure`.
Не трогать: include/ovr/memory/memory_backend.hpp, src/memory/fake_backend/**, src/memory/rpm_backend/** (это T006), src/state/** (это T007/T008).
```

### T006W — бэкенд Linux: `process_vm_readv`

```text
ID: T006W
Milestone: M2. Зависимости: T001, T004, T005, T006.
Цель:
  Дать Linux-сборке настоящий бэкенд чтения, где вся пачка диапазонов уходит за один syscall —
  иначе на Linux бюджет §5 недостижим, а вся приёмка (`ARCHITECTURE.md:544`) идёт на Linux.
Вход:
  - ARCHITECTURE.md:114 — 3.1 IProcessMemory (тот же интерфейс, что реализовал T004).
  - ARCHITECTURE.md:165 — 3.4 (читать, чтобы не сломать ожидания TypedReader к флагам ok).
  - include/ovr/memory/memory_backend.hpp — заголовок T004, менять нельзя.
  - src/memory/rpm_backend/read_process_memory_backend.cpp — реализация-образец от T006.
  - RESEARCH.md:283 — сравнение process_vm_readv / /proc/pid/mem и выбор варианта.
  - ARCHITECTURE.md:416 — §5: <= 0.6 мс на всю пачку чтения.
Выход:
  - src/memory/vm_readv_backend/process_vm_readv_backend.cpp — (класс ProcessVmReadvBackend)
  - src/memory/vm_readv_backend/CMakeLists.txt — (своя цель ovr_memory_vmreadv, линкует ovr_core)
  - tests/unit/test_process_vm_readv_backend.cpp — (бэкенд-контракт на своём же процессе)
Контракт (из ARCHITECTURE.md:114):
  Класс реализует IProcessMemory из 3.1 без изменения заголовка. read_batch(reqs) делает
  ровно ОДИН вызов process_vm_readv(2) со всеми iovec'ами: remote iovec по одному на диапазон,
  local iovec в предоставленный caller-буфер. short read (возврат < запрошенного) трактуется как
  частичный успех: флаги ok выставляются по фактически прочитанным байтам, хвост диапазона не
  трогать. Возврат -1 с EAGAIN/EINTR — ретраить не более 3 раз, после чего все диапазоны ok==false.
  module_base(name) — по карте загруженных модулей процесса (чтение один раз при attach, кэш).
  Никакого чтения /proc/<pid>/mem в рантайме (медленнее и рвёт пакетность, RESEARCH.md:283).
Ограничения:
  Тест обязан читать память СОБСТВЕННОГО процесса (статический массив-образец) — хост в тесте
  не нужна, /proc чужого процесса в тестах запрещён. Своя директория и свой CMakeLists.txt —
  единственное изменение сборки: общего файла с T006 больше нет (PRINCIPLES P7).
DoD / тест:
  1) тест объявляет `static const std::array<std::byte, 4096> kSample`, делает из него local iovec
     и читает 8 диапазонов у собственного pid → все ok, байты совпадают с образцом;
  2) заведомо неверный адрес (0x1) в середине пачки → этот диапазон ok==false, соседние ok==true;
  3) счётчик вызовов: в тесте проверяется, что реализация использует ровно один syscall на пачку
     (`grep -c process_vm_readv src/memory/vm_readv_backend/process_vm_readv_backend.cpp` == 1);
  4) тесты T005 (FakeBackend) и T006 (typed_reader) остаются зелёными;
  5) `cmake --build build && ctest --test-dir build --output-on-failure` — полный прогон зелёный;
  6) `grep -rn "process_vm_writev\|ptrace" src/memory` → пусто.
Не трогать: include/ovr/memory/memory_backend.hpp, read_process_memory_backend.cpp, typed_reader.*.
```

### T021 — конфиг: JSON + схема + hot-reload между кадрами

```text
ID: T021
Milestone: M2. Зависимости: T001, T002.
Цель:
  Выгнать все настраиваемые значения из кода в валидируемый файл, чтобы M5–M6 добавляли фичи
  без правки типов конфига, а кривой конфиг не ронял наблюдатель, а откатывался к дефолтам.
Вход:
  - ARCHITECTURE.md:307 — раздел 3.10, точные сигнатуры (цитата ниже).
  - RESEARCH.md:218 — образец конфига пользователя: ключи берутся ровно оттуда, новых не выдумывать.
  - RESEARCH.md:160 — список convar'ов и их легальные диапазоны (для camera/weather).
  - ARCHITECTURE.md:461 — риск R8 (cl_weather только 0-10, experimental).
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
Контракт (из ARCHITECTURE.md:307):
  namespace ovr::config
    struct Config { bool overlay_enabled{}; Key menu_hotkey{}; /* combo, last_hit_helper,
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
Ограничения:
  Тип Key брать из ARCHITECTURE.md:284; если include/ovr/input/input_backend.hpp ещё не создан
  (T019 в M6), объявить Key в config.hpp как `enum class Key : std::uint16_t` с комментарием-
  ссылкой на 3.9 и НЕ создавать заголовки input/ — это файл T019, конфликт дороже.
  Никаких переменных окружения, реестра, `%APPDATA%`; путь передаётся аргументом.
DoD / тест:
  1) good.json → load()==true, поля совпадают с RESEARCH.md:218 (сверяется поле за полем);
  2) bad_type.json, unknown_key.json, out_of_range.json → load()==false, error непустая,
     все поля out == дефолтам (не частичное состояние!);
  3) round-trip: save() затем load() в тот же путь → Config побайтово равен (operator== в тесте);
  4) validate() ловит custom_distance==2500, cast_delay_ms==250, cl_weather==11;
  5) `python3 scripts/check_contracts.py config.schema.json` → exit 0;
  6) `grep -rn "getenv\|_NSGet\|HKEY" src/config` → пусто;
  7) `cmake --build build && ctest --test-dir build -R config --output-on-failure`.
Не трогать: include/ovr/input/**, src/input/** (это T019), contracts/** , scripts/check_contracts.py.
```

### T015 — SHM-снапшоты и бенчмарк бюджета кадра

```text
ID: T015
Milestone: M2. Зависимости: T001, T005, T006W, T021.
Цель:
  Замерить бюджет кадра ДО появления фич и завести ring буфер снапшотов, чтобы позже можно
  было отделить «наблюдатель» и «рендер» без сокетов и без второго обязательного процесса.
Вход:
  - ARCHITECTURE.md:498 — раздел 7.2, точные сигнатуры (цитата ниже).
  - ARCHITECTURE.md:416 — §5: все числовые лимиты и правила no-alloc (главный DoD задачи).
  - ARCHITECTURE.md:486 — 7.1: почему по умолчанию механизм выключен в M0-M7.
  - RESEARCH.md:259 — 9: требования к окну/рендере (потребитель снапшотов появится в T014).
  - include/ovr/state/host_state.hpp — если T007 ещё не сделан, объявить сериализацию по
     ARCHITECTURE.md:182 поверх типов из фикстур; тогда выход помечать not-established.
  - src/memory/fake_backend/** — источник предсказуемых данных для бенчмарка.
Выход:
  - include/ovr/ipc/snapshot_stream.hpp — (заголовок ровно по 7.2)
  - src/ipc/snapshot_stream.cpp — (POSIX shm + SPSC ring на 4 слота, CRC32)
  - src/ipc/CMakeLists.txt — (цель ovr_ipc: только ovr_core + ovr_state)
  - tests/unit/test_snapshot_stream.cpp — (публикация/потребление/пропуск/повреждение слота)
  - tests/bench/bench_frame.cpp — (меряет этапы пайплайна по §5 и печатает таблицу)
  - tests/bench/CMakeLists.txt — (таргет ovr_bench_frame; в ctest только smoke-прогон)
Контракт (из ARCHITECTURE.md:498):
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
  publish() никогда не блокируется: если потребитель отстал, старый слот просто затирается,
  seq монотонно растёт. try_consume(): если нового слота нет => false и out не тронут; если
  checksum не сходится или magic/schema_version не те => false и потребитель рисует прошлый
  снапшот (сам решает, здесь только false). Имя shm — input-параметр как есть, без префиксов;
  деструктор делает shm_unlink. По умолчанию весь ipc выключен (7.1), main.cpp его не добавляет.
Ограничения:
  Никаких сокетов, именованных pipes, второго процесса в тестах. Никаких аллокаций в publish()/
  try_consume() (payload сериализируется в слот на месте). Бенчмарк не имеет права требовать
  хост: данные берутся из FakeBackend (T005) и фикстур `tests/golden/**`, если они уже есть,
  иначе — из локально собранных подставных кадров. Бенчмарк не спит больше 200 мс суммарно.
DoD / тест:
  1) publish/try_consume 1000 раз подряд → seq монотонны, все checksum валидны, потерь нет;
  2) потребитель спит «руками» (не вызывать try_consume) 6 публикаций подряд → следующий consume
     отдаёт САМЫЙ НОВЫЙ снапшот, а publish() ни разу не вернул false;
  3) портить 1 байт payload в слоте → try_consume() возвращает false, out не изменён;
  4) портить magic/schema_version → false; два reader'а на одном имени не создаются (shm_open
     с новым именем в тесте не конфликтует);
  5) no-alloc: publish/try_consume под счётным аллокатором из T005 → 0 аллокаций после прогрева;
  6) `./build/tests/bench/ovr_bench_frame` печатает таблицу и финальная строка вида
     `total=<x> ms budget=1.8 ms ok|FAIL`; числовые пороги из §5: read<=0.600, parse<=0.400,
     features<=0.300, draw<=0.500, total<=1.800 мс; при FAIL задача НЕ принимается;
  7) smoke-прогон бенча внутри ctest (одна итерация, без порогов) зелёный;
  8) `ctest --test-dir build --output-on-failure` — весь набор зелёный.
Не трогать: src/main.cpp (кроме case'а, если §5 требует — иначе не трогать вовсе), src/render/**,
src/overlay/**, include/ovr/state/host_state.hpp (это T007).
```

---

# M3 — Состояние хоста: структура, парсер, ресурс

Здесь «сырые байты» превращаются в `HostState`, с которым дальше работают фичи. Ключевая
идея: парсер ничего не знает про отрисовку и умеет честно говорить «не знаю» через
`validity_bits` (`ARCHITECTURE.md:182`). Поле без подтверждённого смещения = sentinel,
а не угаданное число (`RESEARCH.md:100`).

### T007 — HostState/Agent: публичные типы состояния

```text
ID: T007
Milestone: M3. Зависимости: T001, T006, T021.
Цель:
  Дать всему проекту единственный публичный тип «что сейчас в хосте» — HostState,
  ActorSnapshot, Agent — чтобы фичи и отрисовка не трогали память.
Вход:
  - ARCHITECTURE.md:182 (§3.5) — ActorSnapshot и HostState заданы дословно, трогать нельзя.
  - ARCHITECTURE.md:114 (§3.1), ARCHITECTURE.md:126 (§3.2) — чем парсер читает память.
  - ARCHITECTURE.md:146 (§3.3) — откуда берутся смещения, `is_stale()`.
  - ARCHITECTURE.md:41 (§2) — state/ не имеет права знать про render/, overlay/, windows.h.
  - Заголовки T001 (`include/ovr/core/`), T006 (`include/ovr/memory/typed_reader.hpp`) — читать как данность.
  - RESEARCH.md:127 (§5) — что реально подтверждено в билде 6985, чего нет.
Выход:
  - include/ovr/state/host_state.hpp (ActorSnapshot и HostState — буква в букву из §3.5; Agent — ниже).
  - include/ovr/state/validity.hpp (битовые маски validity_bits: по одному биту на поле, enum `FieldBit`,
    `bool field_ok(std::uint64_t bits, FieldBit)`) — контракт фиксируется этой задачей в форме ниже,
    ревьюер перенесёт в §3 комментарием.
  - src/state/host_state.cpp (реализация `to_string`/дебажного описания Agent и ActorSnapshot, без IO).
  - tests/unit/test_state.cpp ( POD-семантика, копирование, default-значения, size/align стабильны).
Контракт (из ARCHITECTURE.md:182):
  struct ActorSnapshot { core::Vec3 pos; int team; int health, max_health, mana, max_mana;
                        int level; std::uint32_t flags; bool alive, visible_to_us; };
  struct HostState { int tick_client, tick_server; double current_time; int map_width, map_height;
                     core::Vec3 camera_origin; core::Mat4x4 view_matrix; float fov_deg, aspect,
                     znear, zfar; std::span<ActorSnapshot> actors; std::span<Agent> agents; // view в арену
                     std::uint64_t validity_bits; bool is_background_map; int sign_on_state; };
  Поля ActorSnapshot/HostState и их порядок фиксированы §3.5; добавлять поля в ActorSnapshot нельзя.
  `Agent` в §3.5 назван, но не определён — контракт фиксируется этой задачей в форме ниже,
  ревьюер перенесёт в §3 комментарием:
    struct Agent { std::uint32_t raw_index; core::Vec3 pos; int team;
                  int health, max_health; int gold, last_hits, xp;
                  std::uint32_t flags; bool alive, is_actor, is_minion, is_guard,
                            is_building, is_beacon; std::uint16_t entity_type; };
  `entity_type` — числовой класс из дампа T011 (`RESEARCH.md:47`), переводить в bool-флаги
  только по таблице из `contracts/fields.client-6985.json`; своих догадок о class ID нет.
 Sentinel-семантика: любое int-поле, смещение которого отсутствует в
  `contracts/fields.client-6985.json`, равно -1; любой bool в таком случае false;
  `gold/last_hits/xp` помечены как not-established до появления дампа (`RESEARCH.md:100`).
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
  5) `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j &&
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
  - ARCHITECTURE.md:182 (§3.5) — `HostState parse_frame(const memory::RawFrame&, const
    offsets::OffsetRegistry&, state::FrameArena&);` — сигнатура с ареной, P4.
  - ARCHITECTURE.md:165 (§3.4) — TypedReader, `read_or_false`; ARCHITECTURE.md:146 (§3.3) — OffsetRegistry.
  - ARCHITECTURE.md:126 (§3.2) — RawFrame: что уже прочитано куском и почему нельзя читать по байту.
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
  - tests/unit/test_parse_frame.cpp (парсинг на фейковом RawFrame: подставные байты -> ожидаемый HostState).
Контракт (из ARCHITECTURE.md:182):
  HostState parse_frame(const memory::RawFrame&, const offsets::OffsetRegistry&, state::FrameArena&);
  Ровно один вызов за кадр. Возврат по значению фиксирован §3.5, поэтому внутри парсера держится
  арена `state::FrameArena` (объявляет T007) живёт между кадрами: parse_frame пишет в готовые
  слоты и возвращает span-ы; после первого кадра аллокаций ноль (PRINCIPLES P4).
  `offsets.is_stale() == true` => возвращать HostState с `validity_bits == 0` и пустыми span-ами,
  ничего не выдумывая. Поле считается достоверным только если его смещение есть в реестре И
  чтение удалось; иначе соответствующий бит `FieldBit` (T007) снят.
Ограничения:
  - Никаких литералов смещений в .cpp: только имена из OffsetRegistry (`RESEARCH.md:100`).
  - `entityCount` использовать как нижнюю оценку, а не как границу обхода; границу задавать
    именованной константой и ограничивать сверху, чтобы не читать за карту.
  - Никаких аллокаций, итераторов-владений указателями хоста, sleep, /proc, интернета в парсере.
  - Один процесс, один поток (§5:416); потокобезопасность не требуется, гонки не добавлять.
DoD / тест:
  1) test_parse_frame.cpp строит RawFrame из `std::array<std::uint8_t, N>` с вручную расставленными
     значениями и проверяет: 2 актора + 3 пажа -> `actors.size()==2`, `agents.size()`>=3, координаты совпали.
  2) Тест «empty slot run»: слоты 0..3 заполнены, 4..20 пустые -> парсер останавливается и не читает дальше лимита.
  3) Тест «stale registry»: реестр с другим `ClientVersion` -> `validity_bits == 0`, actors и agents пусты.
  4) Тест «missing field»: смещения `m_iHealth` убрано из фикстуры -> бит снят, `health == -1`.
  5) Тест на аллокации: счётный аллокатор вокруг 100 вызовов parse_frame даёт 0 аллокаций после первого кадра.
  6) `grep -RE "[^0-9][0-9]{6,}[^0-9]" src/state/parse_frame` — пусто (нет хардкода смещений).
  7) `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j &&
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
  - ARCHITECTURE.md:182 (§3.5), ARCHITECTURE.md:126 (§3.2) — что именно сериализуем в эталон.
  - ARCHITECTURE.md:588 (§8.2) — требования приёмки; ARCHITECTURE.md:416 (§5) — бюджеты.
  - RESEARCH.md:100 (§4, методология) — правило «поле принимается по >=2 источникам или своему дампу».
  - RESEARCH.md:218 (§8) — как снимать дампы и почему нельзя просто запомнить адреса.
  - src/state/parse_frame/** (T008), contracts/evidence/client-6985/ (T011) — читать как данность.
Выход:
  - tests/golden/frame-6985-menu.json (дамп одного кадра в меню: тики, sign_on_state, размеры окна) —
    (снимается один раз живьём и коммитится; потом только читается).
  - tests/golden/frame-6985-hud.json (дамп кадра в матче: 10 акторов + часть пажей).
  - tests/golden/raw/frame-6985-menu.bin, tests/golden/raw/frame-6985-hud.bin (сырые байты RawFrame,
    чтобы тест не зависел от LIVE-памяти).
  - tests/golden/README.md (runbook P8/Q9: как одной командой на Windows снять дампы живьём,
    закоммитить их с MANIFEST.sha256, переснять после патча и что делать при провале теста).
  - tests/golden/MANIFEST.sha256 — (sha256 на каждый .bin и .json; приёмка сверяет хэш, RESEARCH.md:115)
  - tests/unit/test_golden_state.cpp (читает .bin, зовёт parse_frame, сверяет HostState по полям с .json).
Контракт (из ARCHITECTURE.md:182):
  Парсер из §3.5 вызывается ровно один раз на каждый golden-дамп; сравнение полей — только через
  публичные поля HostState/ActorSnapshot/Agent из T007. JSON-эталон повторяет имена полей §3.5 1-в-1;
  для недостоверных полей в эталоне стоит `null`, и тест требует `validity_bits` со снятым битом.
  Контракт формата фикстуры фиксируется этой задачей в форме ниже, ревьюер перенесёт в §3 комментарием:
  { "client_version": <int>, "raw": "<имя .bin>", "state": { "<поле §3.5>": <значение|null>, ... } }
Ограничения:
  - Тестам нельзя читать /proc, память хоста, GPU, интернет и спать (§8.2:588); только файлы из tests/golden/.
  - .bin не генерировать во время теста; генерация — отдельный скрипт, вызываемый вручную.
  - Не хранить в эталоне поля, которые T008 помечает not-established (кроме как `null`).
DoD / тест:
  1) `ctest --test-dir build --output-on-failure --tests-regex golden` проваливается, если вручную
     изменить один байт координаты в tests/golden/raw/frame-6985-hud.bin (проверить локально и вернуть как было).
  2) Тест печатает diff вида `field=m_pos expected=... actual=...` при несовпадении — постранично, а не весь объект.
  3) `python3 -c "import json;json.load(open('tests/golden/frame-6985-hud.json'))"` без ошибок; both .json проходят ту же проверку.
  4) `grep -c "\"client_version\": 6985" tests/golden/*.json` — по 1 в каждом файле.
  5) Тесты проходят при `ctest --test-dir build --output-on-failure` целиком, без запущенного хоста.
Не трогать: src/state/parse_frame/** (только чтение), contracts/**, src/main.cpp.
```

---

# M4 — Отрисовка: кадры, проекция, оверлей

Пиксели отделены от состояния: фичи складывают примитивы в `FrameDraw`, проекция
переводит мир в экран, оверлей рисует. Окно ровно одно, без подсадки модулей и без перехватов —
это условие выживания после патчей (`RESEARCH.md:259`, риск R1 в `ARCHITECTURE.md:461`).

### T012 — FrameDraw: буфер примитивов без аллокаций

```text
ID: T012
Milestone: M4. Зависимости: T001, T015.
Цель:
  Дать фичам единственный способ «хочу что-то нарисовать»: очередь примитивов с z-order'ом,
  которая за кадр выделяет память ровно один раз.
Вход:
  - ARCHITECTURE.md:206 (§3.6) — Line/Rect/Circle/Poly/Text, DrawItem, фабрики, clear/items/reserve.
  - ARCHITECTURE.md:235 (§3.7) — кто позже превращает мировые координаты в экранные.
  - ARCHITECTURE.md:328 (§3.11) — как фича получает FrameDraw в `run()`.
  - ARCHITECTURE.md:416 (§5) — бюджет draw <= 0.5 мс и no-alloc в горячем пути.
  - ARCHITECTURE.md:41 (§2) — render/ не знает про overlay-бэкенды и windows.h.
Выход:
  - include/ovr/render/frame_draw.hpp (типы и FrameDraw из §3.6 дословно).
  - src/render/frame_draw.cpp (фабрики, clear(), items(), reserve(), порядок z-order).
  - tests/unit/test_render.cpp (семантика примитивов + счётный аллокатор).
Контракт (из ARCHITECTURE.md:206):
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
Ограничения:
  - Никаких вызовов DX/Vulkan/ImGui в src/render/**: это чистый CPU-буфер команд.
  - `std::string_view` в Text обязан указывать на строку с временем жизни дольше кадра
    (тест на literal/static-буфер обязателен).
  - Никаких аллокаций в clear() и items(); reserve(0) не должен аллоцировать.
DoD / тест:
  1) Тест со счётным аллокатором: reserve(N), затем N вызовов фабрик -> 0 аллокаций.
  2) Тест: reserve(0) затем 3 примитива -> ровно 1 аллок (рост пула), а не 3.
  3) Тест z-order: items()[i] совпадает с i-м вставленным примитивом по типу и координатам.
  4) Тест: после clear() items().size()==0 и новых аллокаций нет.
  5) `grep -RE "d3d|vulkan|imgui|windows\.h" src/render include/ovr/render` — пусто.
  6) `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j &&
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
  - ARCHITECTURE.md:235 (§3.7) — View, world_to_screen, screen_to_world_ray, семантика координат.
  - ARCHITECTURE.md:182 (§3.5) — откуда View берётся (view_matrix, camera_origin, fov, aspect, near/far).
  - RESEARCH.md:183 (§7) — почему живой матрицы в 6985 пока нет и почему тесты только синтетические.
  - ARCHITECTURE.md:461 (§6, риск R4) — T011 в критическом пути перед T013.
  - src/render/frame_draw.hpp (T012) — читать как данность.
Выход:
  - include/ovr/render/world_to_screen.hpp (View и две функции из §3.7 дословно).
  - src/render/world_to_screen.cpp (реализация на Mat4x4 из core/).
  - tests/unit/test_world_to_screen.cpp (исключительно синтетические матрицы).
Контракт (из ARCHITECTURE.md:235):
  struct View { core::Mat4x4 matrix; core::Vec3 cam_origin; float fov_deg{}, aspect{}, znear{}, zfar{}; bool valid{}; };
  bool world_to_screen(const View&, Vec3 world, Vec2& out_screen);
  bool screen_to_world_ray(const View&, Vec2 screen, Vec3& out_dir);
  Матрица row-major; выходные координаты — пиксели клиентского окна, начало слева сверху;
  false возвращается при `!valid`, при `w <= 0` (точка за камерой) и при выходе за границы окна;
  `screen_to_world_ray` возвращает нормализованный вектор направления, начало луча — `cam_origin`.
  `out_screen` при false не трогать (неопределённое значение запрещено использовать).
Ограничения:
  - Никаких DX/Vulkan/ImGui заголовков; никаких обращений к памяти хоста.
  - Никаких магических констант проекции: всё из View; aspect<=0 или fov_deg<=0 => false.
  - Реальных матриц из хоста в тесты не подставлять (их ещё нет, RESEARCH.md:183).
DoD / тест:
  1) Тест «тождественная матрица + камера в начале координат»: точка (0,0,-1000) даёт экранный центр окна.
  2) Тест «за камерой»: точка (0,0,+1000) => false, out_screen не изменён (проверяется стражем-значением).
  3) Тест round-trip: world_to_screen -> screen_to_world_ray даёт направление, colinearное вектору в точку
     (скалярное сравнение с допуском 1e-4f).
  4) Тест вне окна: x за шириной => false; `valid == false` => false для обеих функций.
  5) Тест симметрии: точка справа от центра даёт x > центра, зеркальная — x < центра.
  6) `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j &&
     ctest --test-dir build --output-on-failure --tests-regex world_to_screen` — зелёные.
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
  - ARCHITECTURE.md:260 (§3.8) — OverlayDesc, IOverlay, create_directx11/create_vulkan.
  - ARCHITECTURE.md:206 (§3.6) — что рисует present(); ARCHITECTURE.md:235 (§3.7) — координаты уже экранные.
  - RESEARCH.md:259 (§9) — требования к окну: без tool-window-мигания, без смены фокуса, GDI запрещено.
  - RESEARCH.md:283 (§10) — какой графический бэкенд выбран как основной и почему Vulkan — второй.
  - ARCHITECTURE.md:461 (§6, риск R1) — никаких перехватов и подсадки модулей.
  - ARCHITECTURE.md:416 (§5) — present <= 0.5 мс CPU на кадр, не блокировать дольше одного кадра.
Выход:
  - include/ovr/overlay/overlay.hpp (OverlayDesc, IOverlay, обе фабрики из §3.8).
  - src/overlay/dx11/dx11_overlay.cpp (реализация IOverlay на Direct3D 11 — единственное окно).
  - src/overlay/vulkan/vulkan_overlay.cpp (скелет IOverlay, собираемый, но не включаемый в рантайм;
    помечен как not-established по RESEARCH.md:283).
  - src/overlay/command_buffer.cpp (FrameDraw -> список вершин/текстура глифов, чистая CPU-логика).
  - src/overlay/window.cpp (создание окна: topmost, layered/transparent, без рамки, без tool-window,
    клик-_through по умолчанию; ввод события мыши только для чтения курсора).
  - third_party/imgui/ (vendored ImGui, статически; third_party/README.md с точной версией и ссылкой).
  - tests/unit/test_overlay_command_buffer.cpp (FrameDraw -> ожидаемый список вершин, без окна и GPU).
Контракт (из ARCHITECTURE.md:260):
  struct OverlayDesc { uint32_t width, height; bool topmost, transparent; std::string_view title; };
  class IOverlay { virtual bool init(const OverlayDesc&)=0; virtual void resize(uint32_t w, uint32_t h)=0;
                   virtual bool present(const render::FrameDraw&)=0;
                   virtual bool next_event(core::Vec2& cursor, bool& pressed)=0; };
  IOverlay* create_directx11(); IOverlay* create_vulkan();
  Одно окно на весь процесс; `init()` вызывается один раз, `resize()` — только при смене размера клиента;
  `present()` синхронизирован с vsync и не должен блокировать дольше одного кадра;
  `next_event()` возвращает false, когда событий нет (не спать, не крутить поток).
  Меню по хоткею работает, даже когда окно не в фокусе; фокус окну не передавать никогда.
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
  7) `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j &&
     ctest --test-dir build --output-on-failure` — зелёные на Linux без окна (overlay-секция пропускается через метку).
Не трогать: src/render/**, include/ovr/features/** (это T016), src/input/** (это T019).
```

### T016 — Vision: метки акторов и реестр фич

```text
ID: T016
Milestone: M4. Зависимости: T007, T011, T012, T013, T014, T021.
Цель:
  Первая настоящая фича (метки акторов) плюс каркас реестра фич из §3.11, чтобы все последующие
  фичи были «одна директория + один класс + одна строка регистрации».
Вход:
  - ARCHITECTURE.md:328 (§3.11) — IFeature, FeatureRegistry, список запретов для фич.
  - ARCHITECTURE.md:206 (§3.6) и ARCHITECTURE.md:235 (§3.7) — чем и как рисовать.
  - ARCHITECTURE.md:307 (§3.10) — как читать настройки (`markers_enabled`, `marker_box_color`, `marker_distance_units`).
  - ARCHITECTURE.md:182 (§3.5) — HostState/ActorSnapshot как единственный источник данных.
  - RESEARCH.md:183 (§7) и ARCHITECTURE.md:461 (§6, риск R4) — метки не принимаются, пока view-поля не добыты.
  - RESEARCH.md:322 (§12) — что такое «visible_to_us» и почему видимость акторов требует модели FOW (риск R5).
  - ARCHITECTURE.md:379 (§4.1), ARCHITECTURE.md:404 (§4.2) — границы директорий и рёбра зависимостей.
Выход:
  - include/ovr/features/feature.hpp (IFeature из §3.11 дословно).
  - include/ovr/features/feature_registry.hpp + src/features/feature_registry.cpp
    (FeatureRegistry::add/run_all/features; вызов фич в алфавитном порядке по name()).
  - src/features/vision/vision_model.hpp (интерфейс модели FOW: `bool is_visible(const HostState&,
    std::uint32_t raw_index, int team) const;`) — контракт фиксируется этой задачей, ревьюер перенесёт в §3;
    реализация по умолчанию = null-model, которая отвечает «не знаю» и снимает бит достоверности.
  - src/features/vision/vision_model_fow.cpp (заглушка-заготовка на future: компилируется, но
    возвращает not-established; без угадывания формы FOW).
  - src/features/vision/vision_feature.hpp, src/features/vision/vision_feature.cpp
    (класс VisionFeature : public IFeature с name() == "vision").
  - src/main.cpp (ровно одна строка `registry.add(std::make_unique<features::VisionFeature>());`
    внутри `register_features()`).
  - tests/unit/test_vision.cpp (фича на подставном HostState).
Контракт (из ARCHITECTURE.md:328):
  class IFeature { virtual std::string_view name() const=0;
                   virtual void run(const state::HostState&, const config::Config&, render::FrameDraw&)=0; };
  class FeatureRegistry { void add(std::unique_ptr<IFeature>);
                          void run_all(const state::HostState&, const config::Config&, render::FrameDraw&);
                          std::span<IFeature* const> features() const; };
  Фича не читает память, не хранит указатели хоста между кадрами, не аллоцирует в run(),
  не меняет convar-ы. `run_all` обязан вызвать каждую фичу ровно один раз за кадр и не бросать наружу.
  При `markers_enabled == false` фича возвращается до первой отрисовки (0 примитивов).
  Отрисовка меток: рамка (rect), полоса состояния, имя, дистанция в агентах — только примитивами §3.6;
  мировой координатный перевод строго через `world_to_screen` (T013), свои формулы запрещены.
Ограничения:
  - Не рисовать скелет/снап-лайны/радиусы — это T024/T025 (иначе метки не примут из-за R4).
  - `visible_to_us` из ActorSnapshot используется как есть; догадываться о FOW внутри фичи запрещено (R5).
  - Никаких новых процессов/сокетов, никаких потоков; `run()` без аллокаций (тест со счётным аллокатором).
DoD / тест:
  1) Тест «два актора в 400 агентах, один невидимый»: включённые метки дают ровно 3 набора примитивов
     для видимого (rect + text + text) и 0 для невидимого; дистанция в подписи == 400 (+/-1).
  2) Тест «выключено»: `markers_enabled == false` -> items().empty() при непустом HostState.
  3) Тест «за камерой»: актор с w<=0 по матрице View -> 0 примитивов, ошибок нет.
  4) Тест реестра: 2 зарегистрированные фичи -> run_all вызывает обе ровно один раз (счётчики-атомы в тестовых фичах);
     `features().size() == 2`, порядок = алфавитный.
  5) Тест с счётным аллокатором: после первого кадра run_all даёт 0 аллокаций.
  6) `grep -n "registry.add" src/main.cpp` — ровно одна строка (T016 — первая фича, до неё их в файле не было);
     `git`-коммиты не делать.
  7) `python3 scripts/check_includes.py` (если T023 ещё не сделан — проверить вручную по §4.2:404):
     src/features/** не включает memory/, src/state/** не включает render/.
  8) `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j &&
     ctest --test-dir build --output-on-failure` — зелёные без хоста.
Не трогать: src/render/**, src/overlay/**, include/ovr/state/**, contracts/** (кроме чтения).
```
---

# M5 — read-only фичи: рисуем то, что уже есть в HostState

Одна директория на фичу, один `IFeature` (ARCHITECTURE.md:328), один тест, одна строка
`registry.add`. Данные — только из `HostState` (ARCHITECTURE.md:182), рисование — только
примитивами `FrameDraw` (ARCHITECTURE.md:206) и проекцией `world_to_screen` (ARCHITECTURE.md:235).
Реализм фич — RESEARCH.md:218 (§8), имена настроек — RESEARCH.md:245, открытые вопросы —
RESEARCH.md:335 (§13). Почти всё зависит от T011: пока Q1/Q2 не закрыты, фича рисует «не знаю».
Ключи конфига заранее объявлены T021 (PRINCIPLES P6): у фичи есть `<имя_фичи>.enabled` и её
параметры из схемы, master-switch меток — `markers_enabled`. Если нужного ключа в схеме нет,
исполнитель возвращает «контракт неполон» (владелец ключей — T021), а не придумывает имя.

### T017 — Индикатор добивания

```text
ID: T017
Milestone: M5. Зависимости: T007, T011, T012, T013, T016, T021.
Цель:
  Индикатор добивания пажа: полоса состояния плюс отметка порога денай. Точную оценку
  входящего урона не выдумывать: таблица урона в сборке 6985 не подтверждена
  (RESEARCH.md:335 (§13) Q4), поэтому фича отдаёт индикатор, а не вердикт «добить можно /
  нельзя».
Вход:
  - ARCHITECTURE.md:328 (§3.11) — IFeature/FeatureRegistry: что фиче можно и чего нельзя.
  - ARCHITECTURE.md:182 (§3.5) — HostState, Agent, validity_bits: единственный источник данных.
  - ARCHITECTURE.md:206 (§3.6) — FrameDraw: чем рисовать примитивы.
  - ARCHITECTURE.md:235 (§3.7) — world_to_screen: как мировая точка становится экранной.
  - ARCHITECTURE.md:307 (§3.10) — last_hit_helper.{enabled,indicator_color,show_deny_threshold}.
  - ARCHITECTURE.md:379 (§4.1) — файлы задачи: src/features/last_hit/ и tests/unit/test_last_hit.cpp.
  - ARCHITECTURE.md:404 (§4.2) — какие include-рёбра запрещены.
  - RESEARCH.md:218 (§8) — что реально делает индикатор добивания в существующих решениях.
  - RESEARCH.md:335 (§13, Q4) — почему без damage table только индикатор.
Выход:
  - src/features/last_hit/last_hit_model.hpp, src/features/last_hit/last_hit_model.cpp — чистая
    политика: на входе Agent + render::View + флаги конфига, на выходе std::optional<Indicator>;
    без memory/ и без рисования.
  - src/features/last_hit/last_hit_feature.hpp, src/features/last_hit/last_hit_feature.cpp —
    адаптер IFeature, name() == "last_hit".
  - src/main.cpp — одна строка registry.add(...) в конец register_features().
  - tests/unit/test_last_hit.cpp — единственный тест задачи.
Контракт (из ARCHITECTURE.md:328):
  class IFeature {
    virtual std::string_view name() const = 0;
    virtual void run(const state::HostState&, const config::Config&, render::FrameDraw&) = 0;
  };
  last_hit_helper.enabled == false -> run() не добавляет ни одного примитива.
  На одного пажа ровно 3 примитива: rect-подложка, rect заполнения (ширина пропорциональна
  health/max_health), text с числом health.
  last_hit_helper.show_deny_threshold == true -> ровно 4-й примитив: text со строкой "?" —
  точное число денай-порога без damage table не установлено (Q4).
  world_to_screen вернул false (w <= 0, точка вне окна) -> паж пропускается целиком.
  Время брать только из HostState::current_time / tick_client / tick_server.
Ограничения:
  - Оценку входящего урона не выдумывать: damage table не подтверждена (RESEARCH.md:335 (§13) Q4).
  - Только пажи и нейтральные агенты; акторы рисуются в src/features/vision/ (T016), дублировать не нужно.
  - max_health == 0 или снятый validity-бит -> паж пропускается; деления на ноль быть не должно.
  - Чтений памяти в фиче нет: health/max_health берутся из HostState (T007/T008).
  - Convar-ы фича не меняет (ARCHITECTURE.md:328); писать в них умеет только T022.
  - В run() ноль аллокаций: буферы готовятся в конструкторе либо через reserve().
  - Новых ключей конфига не вводить; не хватает — вернуть «контракт неполон» (владелец T021).
DoD / тест:
  1) Фикстура: паж health/max_health = 30/100 -> ровно 3 примитива; ширина rect заполнения
     равна 30% ширины подложки с погрешностью +/-1 пиксель.
  2) Тот же паж и show_deny_threshold == true -> ровно 4 примитива, последний — text со строкой "?".
  3) last_hit_helper.enabled == false -> draw.items().empty() == true.
  4) Фикстура «за камерой» (w <= 0) -> 0 примитивов, исключений нет.
  5) Тест со счётным аллокатором: после первого кадра run() даёт 0 аллокаций.
  6) grep -n "registry.add" src/main.cpp — строк registry.add стало на одну больше, чем до начала задачи; git-коммиты не делать.
  7) python3 scripts/check_includes.py (если T023 ещё не сделан — проверить вручную по §4.2:404):
     src/features/last_hit/** не включает memory/ и render-бэкенды.
  8) cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j &&
     ctest --test-dir build --output-on-failure — зелёные без хоста.
Не трогать: src/features/vision/**, src/render/**, src/overlay/**, include/ovr/state/**, contracts/** (кроме чтения).
```

### T018 — Таймеры: метки времени, стоянки и скопления

```text
ID: T018
Milestone: M5. Зависимости: T007, T011, T012, T013, T016, T021.
Цель:
  Board таймеров: для каждой наблюдаемой сущности (метка, стоянка, точка накопления) показать, сколько
  осталось до следующего появления. Интервал не берётся из вики — он измеряется по тикам
  HostState; константы хоста вида «метки каждые 180 секунд» не выдумывать.
Вход:
  - ARCHITECTURE.md:328 (§3.11) — IFeature: фича не читает память и не аллоцирует в run().
  - ARCHITECTURE.md:182 (§3.5) — HostState: tick_client/tick_server/current_time, agents, validity_bits.
  - ARCHITECTURE.md:206 (§3.6) — FrameDraw: примитивы, reserve(), порядок вставки.
  - ARCHITECTURE.md:235 (§3.7) — world_to_screen: привязка слота к экранной позиции.
  - ARCHITECTURE.md:307 (§3.10) — vision_tracker.beacon_timer_overlay как master-switch board'а.
  - ARCHITECTURE.md:379 (§4.1) — файлы задачи: src/features/timers/ и tests/unit/test_timers.cpp.
  - ARCHITECTURE.md:404 (§4.2) — запрещённые include-рёбра.
  - RESEARCH.md:218 (§8) — таймеры меток/стоянок/скоплений как реальная фича.
  - RESEARCH.md:335 (§13) — почему часть полей помечена not-established.
Выход:
  - src/features/timers/timer_board.hpp, src/features/timers/timer_board.cpp — чистый
    наблюдатель: каждый кадр получает span по unit'ам и обновляет фиксированный массив слотов
    (capacity 256) без кучи; состояние доступно только через геттеры board().
  - src/features/timers/timers_feature.hpp, src/features/timers/timers_feature.cpp — адаптер
    IFeature, name() == "timers".
  - src/main.cpp — одна строка registry.add(...) в конец register_features().
  - tests/unit/test_timers.cpp — единственный тест задачи.
Контракт (из ARCHITECTURE.md:328):
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
  7) grep -n "registry.add" src/main.cpp — на одну строку больше, чем до начала задачи;
     python3 scripts/check_includes.py (или вручную по §4.2:404): src/features/timers/** не тянет
     memory/** и render-бэкенды; git-коммиты не делать.
  8) cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j &&
     ctest --test-dir build --output-on-failure — зелёные без хоста.
Не трогать: src/features/vision/**, src/render/**, src/overlay/**, include/ovr/state/**, contracts/** (кроме чтения).
```

### T024 — Skeleton: скелет агента по парам костей

```text
ID: T024
Milestone: M5. Зависимости: T007, T011, T012, T013, T016, T021.
Цель:
  Костяк агента: набор отрезков между парами костей, спроецированных на экран. Таблица пар
  костей — это данные, а не код: она ссылается на имена полей из реестра T011. Сами m_*-поля
  в сборке 6985 не подтверждены (RESEARCH.md:335 (§13) Q1), поэтому до закрытия T011 фича
  рисует «не знаю» (ноль примитивов), а не угаданный скелет.
Вход:
  - ARCHITECTURE.md:328 (§3.11) — IFeature/FeatureRegistry: что фиче можно и чего нельзя.
  - ARCHITECTURE.md:182 (§3.5) — HostState, Agent, validity_bits: единственный источник данных.
  - ARCHITECTURE.md:206 (§3.6) — FrameDraw: чем рисовать line.
  - ARCHITECTURE.md:235 (§3.7) — world_to_screen: как мировая точка становится экранной.
  - ARCHITECTURE.md:307 (§3.10) — Config: чтение настроек; своих ключей у фичи нет.
  - ARCHITECTURE.md:379 (§4.1) — файлы задачи: src/features/skeleton/ и tests/unit/test_skeleton.cpp.
  - ARCHITECTURE.md:404 (§4.2) — какие include-рёбра запрещены.
  - RESEARCH.md:183 (§7) — какие view-поля нужны меткам и что из них подтверждено.
  - RESEARCH.md:218 (§8) — skeleton/joints в существующих решениях.
  - RESEARCH.md:335 (§13, Q1) — почему без m_*-полей скелет не рисуется.
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
Контракт (из ARCHITECTURE.md:328):
  class IFeature {
    virtual std::string_view name() const = 0;
    virtual void run(const state::HostState&, const config::Config&, render::FrameDraw&) = 0;
  };
  markers_enabled == false -> run() не добавляет ни одного примитива.
  Одна пара костей, для обеих точек world_to_screen вернул true -> ровно 1 line.
  Таблица пар пуста (поля не подтверждены) -> 0 примитивов и в описании результата
  пометка not-established.
  Проецировать точки можно только через world_to_screen (ARCHITECTURE.md:235); если для пары
  хотя бы одна точка вернула false (w <= 0, вне окна) -> пара пропускается целиком.
  Время брать только из HostState::current_time / tick_client / tick_server.
Ограничения:
  - Пары костей описывать ссылкой на имена полей T011, а не числами: литералы смещений
    >= 100000 запрещены (ARCHITECTURE.md:146).
  - Имя в реестре фиксировано: name() == "skeleton", переименовывать нельзя.
  - Ключи только объявленные T021 (P6): master-switch markers_enabled (RESEARCH.md:245);
    отдельный ключ цвета/толщины -> «контракт неполон», схему правит только T021.
    отдельная настройка цвета/толщины -> вернуть «контракт неполон», владелец ключей T021.
  - Convar-ы фича не меняет (ARCHITECTURE.md:328); писать в них умеет только T022.
  - В run() ноль аллокаций: буферы отрезков готовятся в конструкторе либо через reserve().
DoD / тест:
  1) Фикстура: агент + таблица из 3 пар костей, все точки внутри кадра -> ровно 3 line.
  2) Таблица пар пуста -> draw.items().empty() == true, в описании результата not-established.
  3) У одной из пар одна точка за камерой (w <= 0) -> ровно 2 line, исключений нет.
  4) markers_enabled == false -> draw.items().empty() == true.
  5) Тест со счётным аллокатором: после первого кадра run() даёт 0 аллокаций.
  6) grep -n "registry.add" src/main.cpp — строк registry.add стало на одну больше, чем до начала задачи; git-коммиты не делать.
  7) python3 scripts/check_includes.py (если T023 ещё не сделан — проверить вручную по §4.2:404):
     src/features/skeleton/** не включает memory/ и render-бэкенды.
  8) cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j &&
     ctest --test-dir build --output-on-failure — зелёные без хоста.
Не трогать: src/features/vision/**, src/render/**, src/overlay/**, include/ovr/state/**, contracts/** (кроме чтения).
```

### T025 — Snap lines: линия до цели приказа

```text
ID: T025
Milestone: M5. Зависимости: T007, T011, T012, T013, T016, T021.
Цель:
  Линия от выбранного агента к объекту его текущего приказа (цель атаки/перемещения).
  Проверку «цель нас видит» делать не трасскастингом по геометрии карты, а через модель
  видимости T016: она работает на данных FOW (RESEARCH.md:335 (§13) Q3), а не на предположении,
  что код хоста можно воспроизвести снаружи.
Вход:
  - ARCHITECTURE.md:328 (§3.11) — IFeature/FeatureRegistry.
  - ARCHITECTURE.md:182 (§3.5) — HostState, Agent, order-поля, validity_bits.
  - ARCHITECTURE.md:206 (§3.6) — FrameDraw: line.
  - ARCHITECTURE.md:235 (§3.7) — world_to_screen для обоих концов линии.
  - ARCHITECTURE.md:307 (§3.10) — Config; своих ключей у фичи нет.
  - ARCHITECTURE.md:379 (§4.1) — src/features/snap_lines/ и tests/unit/test_snap_lines.cpp.
  - ARCHITECTURE.md:404 (§4.2) — запрещённые include-рёбра.
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
Контракт (из ARCHITECTURE.md:328):
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
Ограничения:
  - Поле цели приказа берётся из T011; если оно не подтверждено -> not-established и 0 line.
  - Имя в реестре фиксировано: name() == "snap_lines".
  - Ключи только объявленные T021 (P6): master-switch markers_enabled; отдельный ключ -> вернуть
    «контракт неполон», а не дописывать его в config.schema.json самому.
    «контракт неполон», владелец T021.
  - Model видимости подключается только как заголовок src/features/vision/vision_model.hpp (T016).
  - Convar-ы не менять; в run() ноль аллокаций.
DoD / тест:
  1) Фикстура: агент + цель приказа, обе точки в кадре, is_visible == true -> ровно 1 line.
  2) null-модель видимости -> draw.items().empty() == true (это «не знаю», а не «не видно»).
  3) Цель за камерой (w <= 0) -> draw.items().empty() == true, исключений нет.
  4) markers_enabled == false -> draw.items().empty() == true.
  5) Тест со счётным аллокатором: после первого кадра run() даёт 0 аллокаций.
  6) grep -n "registry.add" src/main.cpp — строк registry.add стало на одну больше, чем до начала задачи; git-коммиты не делать.
  7) python3 scripts/check_includes.py (если T023 ещё не сделан — проверить вручную по §4.2:404):
     src/features/snap_lines/** не включает memory/ и render-бэкенды.
  8) cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j &&
     ctest --test-dir build --output-on-failure — зелёные без хоста.
Не трогать: src/features/vision/**, src/render/**, src/overlay/**, include/ovr/state/**, contracts/** (кроме чтения).
```

### T026 — Fog overlay: сетка «кто кого видит»

```text
ID: T026
Milestone: M5. Зависимости: T007, T011, T012, T013, T016, T021.
Цель:
  Полупрозрачная сетка поверх карты: для каждой клетки статус «видно нам / видно противнику /
  не знаю». Размер клетки — параметр чистой модели (число агентов), а не convar и не смещение.
  Аналог существующих `src-F`-решений (RESEARCH.md:218 §8); статус «не знаю» обязателен,
  потому что данные FOW в сборке 6985 подтверждены лишь частично (RESEARCH.md:335 (§13) Q3).
Вход:
  - ARCHITECTURE.md:328 (§3.11) — IFeature/FeatureRegistry.
  - ARCHITECTURE.md:182 (§3.5) — HostState::map_width/map_height: отсюда размер сетки.
  - ARCHITECTURE.md:206 (§3.6) — FrameDraw: rect.
  - ARCHITECTURE.md:235 (§3.7) — world_to_screen: corners клетки -> экранного четырёхугольника.
  - ARCHITECTURE.md:307 (§3.10) — Config; своих ключей у фичи нет.
  - ARCHITECTURE.md:379 (§4.1) — src/features/fow_overlay/ и tests/unit/test_fow_overlay.cpp.
  - ARCHITECTURE.md:404 (§4.2) — запрещённые include-рёбра.
  - RESEARCH.md:218 (§8) — map vision / fog overlay в существующих решениях.
  - RESEARCH.md:335 (§13, Q3) — FOW подтверждён частично: статус partial = «не знаю».
Выход:
  - src/features/fow_overlay/fow_grid.hpp, src/features/fow_overlay/fow_grid.cpp — чистая
    модель: HostState + модель видимости T016 + размер клетки -> таблица статусов клеток;
    без memory/ и без рисования.
  - src/features/fow_overlay/fow_overlay_feature.hpp, src/features/fow_overlay/fow_overlay_feature.cpp —
    адаптер IFeature, name() == "fow_overlay".
  - src/main.cpp — одна строка registry.add(...) в конец register_features().
  - tests/unit/test_fow_overlay.cpp — единственный тест задачи.
Контракт (из ARCHITECTURE.md:328):
  class IFeature {
    virtual std::string_view name() const = 0;
    virtual void run(const state::HostState&, const config::Config&, render::FrameDraw&) = 0;
  };
  Число клеток = ceil(map_width / cell) * ceil(map_height / cell), где map_width/map_height —
  из HostState (ARCHITECTURE.md:182); cell — аргумент модели, по умолчанию 64.
  Ровно 1 rect на клетку со статусом «видно нам» или «видно противнику»; для клетки со
  статусом «не знаю» -> 0 rect.
  markers_enabled == false -> run() не добавляет ни одного примитива.
  Клетка целиком за камерой (все углы world_to_screen == false) -> 0 rect для этой клетки.
  Осевшие validity-биты HostState -> сетка пересчитывается, устаревшие статусы не рисовать.
Ограничения:
  - Статус клетки получать только через is_visible(...) из T016; своих расчётов FOW не писать.
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
  6) grep -n "registry.add" src/main.cpp — строк registry.add стало на одну больше, чем до начала задачи; git-коммиты не делать.
  7) python3 scripts/check_includes.py (если T023 ещё не сделан — проверить вручную по §4.2:404):
     src/features/fow_overlay/** не включает memory/ и render-бэкенды.
  8) cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j &&
     ctest --test-dir build --output-on-failure — зелёные без хоста.
Не трогать: src/features/vision/**, src/render/**, src/overlay/**, include/ovr/state/**, contracts/** (кроме чтения).
```

### T027 — Range displays: кольца радиусов

```text
ID: T027
Milestone: M5. Зависимости: T007, T011, T012, T013, T016, T021.
Цель:
  На экране рисуются кольца: радиус воздействия, агр-радиус башен, дистанция рывка. Каталог —
  RESEARCH.md:229 (§8) «Custom Range Displays»: фича помечена R+W, W-вариант через convar
  дешевле. Здесь делается только R-часть (кольцо рисуем сами); запись `cv.range_display` —
  вне объёма блока, см. Ограничения.
Вход:
  - ARCHITECTURE.md:328 (§3.11) — IFeature/FeatureRegistry.
  - ARCHITECTURE.md:182 (§3.5) — HostState: pos агентов, camera_origin, view_matrix.
  - ARCHITECTURE.md:206 (§3.6) — FrameDraw: circle, text.
  - ARCHITECTURE.md:235 (§3.7) — world_to_screen: единственная проекция мир -> экран.
  - ARCHITECTURE.md:307 (§3.10) — Config; своих ключей у фичи нет.
  - ARCHITECTURE.md:379 (§4.1) — src/features/range_display/ и tests/unit/test_range_display.cpp.
  - ARCHITECTURE.md:404 (§4.2) — запрещённые include-рёбра.
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
Контракт (из ARCHITECTURE.md:328):
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
  (ARCHITECTURE.md:182); радиус окружности на экране = |A - B| в пикселях.
  Ровно 1 circle + ровно 1 text на одно кольцо. Если world_to_screen вернул false для A или B
  -> 0 примитивов для этого кольца. rings() пуст -> 0 примитивов всего.
  markers_enabled == false -> run() не добавляет ни одного примитива.
Ограничения:
  - Радиусы (воздействие / агр стража / рывок) задаются только именованными константами в
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
  6) grep -n "registry.add" src/main.cpp — строк registry.add стало на одну больше, чем до начала задачи; git-коммиты не делать.
  7) python3 scripts/check_includes.py (если T023 ещё не сделан — проверить вручную по §4.2:404):
     src/features/range_display/** не включает memory/ и render-бэкенды.
  8) cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j &&
     ctest --test-dir build --output-on-failure — зелёные без хоста.
Не трогать: src/features/vision/**, src/render/**, src/overlay/**, include/ovr/state/**, contracts/** (кроме чтения).
```

### T028 — Cooldown tracker: тайминги воздействий

```text
ID: T028
Milestone: M5. Зависимости: T007, T011, T012, T013, T016, T021.
Цель:
  Над воздействиями врагов — строка «готов через N с». Каталог — RESEARCH.md:230 (§8)
  «Cooldown & Mana Tracker»: фича требует цепочки полей воздействий, поэтому MVP — машина состояний на
  тиках HostState, а не выдуманные длительности воздействий.
Вход:
  - ARCHITECTURE.md:328 (§3.11) — IFeature/FeatureRegistry.
  - ARCHITECTURE.md:182 (§3.5) — HostState: tick_client и pos агентов; тики монотонны.
  - ARCHITECTURE.md:206 (§3.6) — FrameDraw: text.
  - ARCHITECTURE.md:235 (§3.7) — world_to_screen: позиция подписи на экране.
  - ARCHITECTURE.md:307 (§3.10) — Config: vision_tracker.show_enemy_cooldowns.
  - ARCHITECTURE.md:379 (§4.1) — src/features/cooldowns/ и tests/unit/test_cooldowns.cpp.
  - ARCHITECTURE.md:404 (§4.2) — запрещённые include-рёбра.
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
Контракт (из ARCHITECTURE.md:328):
  class IFeature {
    virtual std::string_view name() const = 0;
    virtual void run(const state::HostState&, const config::Config&, render::FrameDraw&) = 0;
  };
  struct AbilitySample { std::uint32_t raw_index; std::uint32_t slot; bool available; };
  void cooldown_machine::observe(std::uint64_t tick, std::span<const AbilitySample> samples);
  std::optional<std::uint64_t> cooldown_machine::ready_at(std::uint32_t raw_index,
      std::uint32_t slot) const;
  observe() вызывается ровно один раз за кадр, tick = HostState::tick_client (ARCHITECTURE.md:182).
  Переход true -> false запоминает тик каста; переход false -> true измеряет длительность
  кулдауна и запоминает её как оценку для пары (raw_index, slot). Повторный каст на тике F ->
  ready_at == F + последняя измеренная длительность, погрешность +-1 тик.
  Если измеренной длительности для пары ещё нет -> ready_at возвращает std::nullopt, и адаптер
  рисует строку «???» вместо числа.
  Ровно 1 text на пару (raw_index, slot); позиция — world_to_screen от pos агента, false ->
  0 примитивов. markers_enabled == false или show_enemy_cooldowns == false -> 0 примитивов.
Ограничения:
  - Длительности воздействий не хардкодить и не брать «из головы»: их в контракте нет
    (RESEARCH.md:335 (§13) Q2). Единственный источник — измеренные переходы available.
  - observe() не знает ни про память, ни про FrameDraw; вся логика времени — только в тиках,
    без wall-clock и без sleep.
  - Сэмплы адаптер строит из HostState; если полей воздействий там нет -> observe() вызывается с
    пустым span и 0 примитивов (не догадываться).
  - Не путать с T018 timers: там таймеры появления/скоплений, здесь — кулдауны воздействий.
DoD / тест:
  1) Фикстура: available == false на тике 100, true на тике 160 -> измеренная длительность 60;
     снова false на тике 200 -> ready_at == 260 (+-1).
  2) Пара без измеренной длительности -> ready_at == std::nullopt и ровно 1 текст «???».
  3) 2 агента по 2 воздействия -> ровно 4 text; world_to_screen == false -> 0 примитивов.
  4) markers_enabled == false -> draw.items().empty() == true (при тех же сэмплах observe() продолжается).
  5) Тест со счётным аллокатором: после первого кадра run() даёт 0 аллокаций.
  6) grep -n "registry.add" src/main.cpp — строк registry.add стало на одну больше, чем до начала задачи; git-коммиты не делать.
  7) python3 scripts/check_includes.py (если T023 ещё не сделан — проверить вручную по §4.2:404):
     src/features/cooldowns/** не включает memory/ и render-бэкенды.
  8) cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j &&
     ctest --test-dir build --output-on-failure — зелёные без хоста.
Не трогать: src/features/vision/**, src/render/**, src/overlay/**, include/ovr/state/**, contracts/** (кроме чтения).
```

### T029 — Обманки и зоны истинного обзора

```text
ID: T029
Milestone: M5. Зависимости: T007, T011, T012, T013, T016, T021.
Цель:
  Обманки (иллюзии/клоны) помечаются рамкой и подписью; зоны истинного обзора (предметы обзора) — кольцом,
  если поля подтверждены. Каталог — RESEARCH.md:233 (§8) «обманки и истинный обзор», деривация
  обманок через m_hPlayer.
Вход:
  - ARCHITECTURE.md:328 (§3.11) — IFeature/FeatureRegistry.
  - ARCHITECTURE.md:182 (§3.5) — HostState: Agent-записи, pos, team, flags, alive.
  - ARCHITECTURE.md:206 (§3.6) — FrameDraw: rect, circle, text.
  - ARCHITECTURE.md:235 (§3.7) — world_to_screen: обводка и кольцо.
  - ARCHITECTURE.md:307 (§3.10) — Config: vision_tracker.illusion_highlight.
  - ARCHITECTURE.md:379 (§4.1) — src/features/illusions/ и tests/unit/test_illusions.cpp.
  - ARCHITECTURE.md:404 (§4.2) — запрещённые include-рёбра.
  - RESEARCH.md:183 (§7) — проекция и требования к отрисовке меток.
  - RESEARCH.md:233 (§8) — обманки и предметы обзора: фича `src-D`.
  - RESEARCH.md:245 (§8) — vision_tracker.illusion_highlight — единственный ключ фичи.
  - RESEARCH.md:335 (§13, Q1/Q2) — флаги обманок и предметов обзора в сборке 6985 не подтверждены.
Выход:
  - src/features/illusions/illusion_model.hpp, src/features/illusions/illusion_model.cpp —
    чистая модель: HostState -> raw-индексы обманок и центры зон истинного обзора; без memory/ и без
    рисования.
  - src/features/illusions/illusions_feature.hpp, src/features/illusions/illusions_feature.cpp —
    адаптер IFeature, name() == "illusions".
  - src/main.cpp — одна строка registry.add(...) в конец register_features().
  - tests/unit/test_illusions.cpp — единственный тест задачи.
Контракт (из ARCHITECTURE.md:328):
  class IFeature {
    virtual std::string_view name() const = 0;
    virtual void run(const state::HostState&, const config::Config&, render::FrameDraw&) = 0;
  };
  struct Illusion { std::uint32_t raw_index; core::Vec3 pos; int team; };
  struct Zone { core::Vec3 center; float radius_units; };
  void illusion_model::illusions(const state::HostState&, std::vector<Illusion>& out);
  void illusion_model::reveal_zones(const state::HostState&, std::vector<Zone>& out);
  Признаком обманки считается только явное поле/флаг из HostState (ARCHITECTURE.md:182). Если
  такого поля в контракте HostState нет -> illusions() возвращает пусто и адаптер не рисует
  ничего; эвристики по имени модели агента запрещены.
  Ровно 1 rect-обводка + ровно 1 text «ill» на обманку; ровно 1 circle на зону истинного обзора.
  world_to_screen вернул false -> 0 примитивов для этого объекта. markers_enabled == false ->
  0 примитивов всего; illusion_highlight == false -> обманки не рисуются, зоны остаются.
  Адаптер держит оба выходных вектора как члены -> после первого кадра run() не аллоцирует.
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
  6) grep -n "registry.add" src/main.cpp — строк registry.add стало на одну больше, чем до начала задачи; git-коммиты не делать.
  7) python3 scripts/check_includes.py (если T023 ещё не сделан — проверить вручную по §4.2:404):
     src/features/illusions/** не включает memory/ и render-бэкенды.
  8) cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j &&
     ctest --test-dir build --output-on-failure — зелёные без хоста.
Не трогать: src/features/vision/**, src/render/**, src/overlay/**, include/ovr/state/**, contracts/** (кроме чтения).
```

### T030 — Алерты и W-политики (камера, погода)

```text
ID: T030
Milestone: M5. Зависимости: T007, T011, T012, T013, T016, T021.
Цель:
  R: строка-алерт «агент в рывке уже летит» и кольцо «меня видит этот враг» для видимых врагов.
  W: две чистые политики — желанная дистанция камеры и желанная погода. Политики только
  возвращают число; никуда не пишут (канал записи — T022).
Вход:
  - ARCHITECTURE.md:328 (§3.11) — IFeature/FeatureRegistry.
  - ARCHITECTURE.md:182 (§3.5) — HostState: pos, team, flags, camera_origin.
  - ARCHITECTURE.md:206 (§3.6) — FrameDraw: text, circle.
  - ARCHITECTURE.md:235 (§3.7) — world_to_screen: якорь алерта и кольцо.
  - ARCHITECTURE.md:284 (§3.9) — для ясности: нажатия клавиш здесь запрещены (это T019).
  - ARCHITECTURE.md:307 (§3.10) — Config: camera.custom_distance (900..2000), camera.smooth_zoom.
  - ARCHITECTURE.md:379 (§4.1) — src/features/alerts/ и tests/unit/test_alerts.cpp.
  - ARCHITECTURE.md:404 (§4.2) — запрещённые include-рёбра.
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
Контракт (из ARCHITECTURE.md:328):
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
  (ARCHITECTURE.md:307); smooth_zoom == false -> значение возвращается сразу, анимаций нет.
  clamp_weather: зажим в [0, 10] (RESEARCH.md:160 §6 cl_weather).
  markers_enabled == false -> run() не добавляет ни одного примитива.
Ограничения:
  - Политики не пишут в память и не меняют convar-ы: канал записи — T022; r_farz = 2 * distance
    — побочный эффект записи, его T022 не трогать здесь (RESEARCH.md:160 §6).
  - Погодного ключа в RESEARCH.md:245 нет -> интеграцию clamp_weather с Config не выдумывать:
    функция принимает int, а привязка к конфигу -> «контракт неполон» (владелец T021).
  - Кольца считать только через is_visible(...) из T016; своей оценки FOW не писать.
  - Никаких нажатий клавиш и мыши (§3.9:284 — это поверхность T019/T020).
DoD / тест:
  1) Фикстура: у врага установлен признак рывка -> ровно 1 text; без признака -> 0 примитивов.
  2) 3 видимых врага (is_visible == true) и 2 невидимых -> ровно 3 circle.
  3) custom_distance == 2500 -> desired_camera_distance() == 2000; custom_distance == 500 -> 900;
     1200 -> 1200.
  4) clamp_weather(-1) == 0, clamp_weather(11) == 10, clamp_weather(5) == 5.
  5) Тест со счётным аллокатором: после первого кадра run() даёт 0 аллокаций.
  6) grep -n "registry.add" src/main.cpp — строк registry.add стало на одну больше, чем до начала задачи; git-коммиты не делать.
  7) python3 scripts/check_includes.py (если T023 ещё не сделан — проверить вручную по §4.2:404):
     src/features/alerts/** не включает memory/ и render-бэкенды.
  8) cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j &&
     ctest --test-dir build --output-on-failure — зелёные без хоста.
Не трогать: src/features/vision/**, src/render/**, src/overlay/**, include/ovr/state/**, contracts/** (кроме чтения).
```

---

# M6 — Ввод, convar-ы и CI: то, что меняет хост

Здесь всё, что выходит за рамки чтения: нажатие клавиш (ARCHITECTURE.md:284 (§3.9)), запись
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
  - ARCHITECTURE.md:284 (§3.9) — точные сигнатуры Key, IInputBackend, create_sendinput/create_uinput.
  - ARCHITECTURE.md:307 (§3.10) — Config: клавиши и задержки приходят из конфига, не зашиваются.
  - ARCHITECTURE.md:379 (§4.1) — include/ovr/input/ и src/input/ принадлежат T019 и T020.
  - ARCHITECTURE.md:404 (§4.2) — какие include-рёбра запрещены между слоями.
  - ARCHITECTURE.md:544 (§8) — тестируемость: реальные бэкенды не должны участвовать в модульный тестах.
  - RESEARCH.md:259 (§9) — перехваты и подсадка модулей в хост = риск; ввод только системными API без перехвата.
  - RESEARCH.md:346 (§13, Q8) — почему SendInput/uinput, а не запись в CInput.
Выход:
  - include/ovr/input/key.hpp — enum class Key : std::uint16_t (Windows VK / Linux codepoints),
    значения совпадают с объявлением Key у T021 (TASKS.md:601).
  - include/ovr/input/input_backend.hpp — IInputBackend { press(Key), release(Key), is_down(Key) }.
  - include/ovr/input/input_source.hpp — обёртка IInputSource поверх бэкенда + белый список
    std::span<const Key>; factory create_input_source(IInputBackend*, std::span<const Key>).
  - src/input/sendinput.cpp — Windows-реализация, ВЕСЬ файл под #ifdef _WIN32.
  - src/input/uinput.cpp — Linux-реализация через /dev/uinput (udev-зависимость не добавлять).
  - src/input/fake_input.cpp — тестовый бэкенд со счётчиками press/release и таблицей is_down.
  - src/input/CMakeLists.txt — цель ovr_input: линкует только ovr_core и ovr_config.
  - tests/unit/test_input_whitelist.cpp, tests/unit/test_input_fake.cpp.
  - корневой CMakeLists.txt не трогать: guarded add_subdirectory(src/input) уже создан T000 (P5).
Контракт (из ARCHITECTURE.md:284):
  enum class Key : std::uint16_t { /* VK / Linux codepoints; MOUSE5 = 5 */ };
  class IInputBackend {
    virtual void press(Key) = 0; virtual void release(Key) = 0;
    virtual bool is_down(Key) const = 0;
  };
  Клавиша вне белого списка -> press/release возвращают false и в бэкенд НЕ обращаются.
  Клавиша внутри списка -> ровно один вызов бэкенда, возврат true.
  Недоступный бэкенд (нет /dev/uinput, нет сессии Win) -> фабрика возвращает nullptr,
  исключений не бросаем; вызывающий код работает без ввода.
  Задержек, потоков и sleep в бэкенде нет: тайминги — ответственность фичи (T020).
Ограничения:
  Никаких перехватов (SetWindowsHookEx, X11 record, eBPF) и записи в CInput (RESEARCH.md:259).
  src/input/** не включает memory/, render/, features/ (§4.2:404); windows.h — только внутри
  #ifdef _WIN32 в sendinput.cpp.
  Тесты не открывают /dev/uinput и не подают реальный ввод: проверяется только fake + whitelist.
  Свой ключ конфига не выдумывать: список клавиш фиче отдаёт вызывающий код из Config (T021).
DoD / тест:
  1) fake: press(K) -> счётчик press==1 и is_down(K)==true; release(K) -> 0/false.
  2) whitelist: K в списке -> 1 вызов бэкенда и true; K вне списка -> false и 0 вызовов бэкенда.
  3) пустой список -> любой Key даёт false, бэкенд не вызван ни разу.
  4) фабрики на недоступном бэкенде -> nullptr без исключений (тест не требует root и GUI).
  5) grep -rn "sleep_for\|nanosleep\|Sleep(\|std::thread" src/input include/ovr/input — пусто.
  6) grep -rn "windows.h" src/input — только в sendinput.cpp и только внутри #ifdef _WIN32.
  7) python3 scripts/check_includes.py (если T023 ещё не сделан — проверить вручную по §4.2:404):
     src/input/** не включает memory/, render/, features/.
  8) cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j &&
     ctest --test-dir build --output-on-failure — зелёные без хоста.
Не трогать: src/features/**, src/render/**, src/overlay/**, include/ovr/config/**, contracts/** (кроме чтения).
```

### T020 — Связка: машина состояний каста (по умолчанию выключена)

```text
ID: T020
Milestone: M6. Зависимости: T007, T012, T016, T019, T021, T022.
Цель:
  Самая рискованная фича каталога (RESEARCH.md:234) — каст по заранее описанной последовательности. Делаем её максимально
  скудной: чистая машина состояний «нажать одну кнопку, выдержать паузу, не спамить», отделённая
  от ввода и от рисования, выключенный по умолчанию. Фича проверяет, что связка HostState ->
  политика -> input работает, и не лезет в специфику акторов.
Вход:
  - ARCHITECTURE.md:328 (§3.11) — IFeature/FeatureRegistry: что фиче можно и чего нельзя.
  - ARCHITECTURE.md:284 (§3.9) — IInputBackend/IInputSource: единственный способ нажать кнопку.
  - ARCHITECTURE.md:182 (§3.5) — HostState: current_time, тики, список агентов — источник времени.
  - ARCHITECTURE.md:206 (§3.6) — FrameDraw: чем показать, что фича сейчас делает (или молчит).
  - ARCHITECTURE.md:307 (§3.10) — combo.enabled, target_lock_key, cast_delay_ms (0..200).
  - ARCHITECTURE.md:379 (§4.1) — src/features/combo/ принадлежит T019/T020.
  - RESEARCH.md:234 (§8) — «Связка» как фича класса I (input) и её риск.
  - RESEARCH.md:346 (§13, Q8) — почему по умолчанию выключено и почему акторов с длинными цепочками не делаем.
Выход:
  - src/features/combo/combo_machine.hpp, combo_machine.cpp — чистая политика: на входе
    HostState-срез + Config-срез, на выходе решение «кастовать/ждать» и номер воздействия из
    белого списка; без memory/, без input/, без render/.
  - src/features/combo/combo_feature.hpp, combo_feature.cpp — адаптер IFeature,
    name() == "combo"; он единственный трогает IInputSource и FrameDraw.
  - src/main.cpp — одна строка registry.add(...) в конец register_features().
  - tests/unit/test_combo.cpp + tests/fixtures/combo/*.json (состояния HostState).
Контракт (из ARCHITECTURE.md:328):
  class IFeature {
    virtual std::string_view name() const = 0;
    virtual void run(const state::HostState&, const config::Config&, render::FrameDraw&) = 0;
  };
  combo.enabled == false (дефолт RESEARCH.md:346) -> за весь кадр ноль вызовов press().
  target_lock_key не нажата -> ноль press(); нажата и машина состояний разрешает каст -> ровно один press().
  Два каста подряд разнесены не менее чем на cast_delay_ms, время берётся из HostState, не из ОС.
  Наведение/выбор цели не угадывается: цель = агент из HostState, помеченный как валидный
  (ARCHITECTURE.md:182); валидных нет -> фича молчит и рисует not-established.
Ограничения:
  Белый список воздействий — только идентификаторы из конфига (RESEARCH.md:245); своих ключей
  не выдумывать, не хватает настройки -> «контракт неполон», владелец ключей T021.
  Специфика акторов с длинными цепочками приказов (RESEARCH.md:234) требует цепочки полей
  воздействий из дампа (Q1, T011):
  пока она не закрыта, реализуется универсальный одно-ключевой каст, а не актор-комбо;
  дописывать таблицы акторов = «контракт неполон».
  Никаких sleep_for/потоков/таймеров: пауза считается из current_time и тиков HostState.
  Input — только через IInputSource из T019 (белый список обязателен), convar-ы не менять.
  В run() ноль аллокаций; внутреннее состояние фичи — её поля, создаваемые до первого кадра.
DoD / тест:
  1) enabled==true, клавиша нажата, пауза истекла -> ровно 1 press(); кадр сразу следом -> 0 press().
  2) enabled==false -> 600 кадров, fake-бэкенд насчитал 0 press(); в FrameDraw ни одного примитива каста.
  3) cast_delay_ms==15 -> второй каст не раньше current_time+15мс; cast_delay_ms==0 -> без паузы.
  4) combo_machine тестируется без memory/render/input (линкуется только с ovr_core+ovr_config);
     grep -rn "memory/\|render/\|input/" src/features/combo/combo_machine.* — пусто.
  5) Тест со счётным аллокатором: после первого кадра run() даёт 0 аллокаций.
  6) grep -rn "sleep_for\|std::thread\|WriteProcess\|convar" src/features/combo — пусто.
  7) python3 scripts/check_includes.py (если T023 ещё не сделан — проверить вручную по §4.2:404):
     src/features/combo/** не включает memory/ и render-бэкенды.
  8) cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j &&
     ctest --test-dir build --output-on-failure — зелёные без хоста.
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
  - ARCHITECTURE.md:307 (§3.10) — Config как единственный источник значений (camera.*, weather).
  - ARCHITECTURE.md:146 (§3.3) — OffsetRegistry: адреса ищутся по имени, литералы запрещены.
  - ARCHITECTURE.md:379 (§4.1) — src/convars/ и tests/unit/test_convar_whitelist.cpp = T022.
  - ARCHITECTURE.md:404 (§4.2) — в contracts/** пишет только T011; сюда — только чтением.
  - ARCHITECTURE.md:461 (§6) — риск R8: cl_weather экспериментальный, коридор сужен заранее.
  - contracts/info.client-6985.json — чем подтверждается, что convar жив в текущей сборке.
Выход:
  - include/ovr/convars/convar_whitelist.hpp — таблица как данные: {имя, уровень риска, мин, макс,
    дефолт, ссылка на RESEARCH.md:160}; ни одной константы-смещения.
  - src/convars/convars.cpp — apply_from_config(const config::Config&, memory::IMemoryBackend*)
    и restore_all(memory::IMemoryBackend*); только эти две функции видны снаружи.
  - src/convars/CMakeLists.txt — цель ovr_convars: ovr_core, ovr_memory, ovr_config.
  - tests/unit/test_convar_whitelist.cpp — единственный тест задачи.
  - src/main.cpp — ровно одна строка convars::apply_from_config(cfg, backend); больше правок нет.
  - корневой CMakeLists.txt не трогать: guarded add_subdirectory(src/convars) уже создан T000 (P5).
Контракт (из ARCHITECTURE.md:307):
  Значения берутся ТОЛЬКО из Config (T021); аргумент-константа допустима лишь для r_farz,
  вычисляемого как 2 * `cv.camera_distance` (RESEARCH.md:160).
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
Ограничения:
  Писать можно только через IMemoryBackend (T006/T006W): никаких своих указателей, своих
  перехватов и прямого ReadProcessMemory/WriteProcessMemory внутри src/convars/**.
  Адрес convar-а получается через OffsetRegistry (ARCHITECTURE.md:146); если имени нет в
  реестре -> не писать и вернуть false с текстом «не Established», угадывать адрес запрещено
  (ARCHITECTURE.md:588 (§8.2)).
  Если одна строка в src/main.cpp не помещается (нет места после загрузки конфига) -> ответ
  «контракт неполон» + дифф ровно из одной строки; переписывать main.cpp нельзя.
  Погодного ключа в образце конфига нет (RESEARCH.md:245): cl_weather брать из
  существующего поля конфига либо вернуть «контракт неполон» (владелец ключей T021).
DoD / тест:
  1) Таблица whitelist == RESEARCH.md:160 построчно (имя, диапазон, риск); лишней строки нет,
     запрещённых имён (`cv.particle_fow`, set_rendering_enabled, `sv_cheats`) в коде нет.
  2) cl_weather==11 в обход validate() -> ни одной записи + сообщение с именем ключа.
  3) apply_from_config на fake-бэкенде записал ровно N значений (N посчитано в тесте); повторный
     вызов с тем же конфигом -> 0 записей.
  4) restore_all() -> все значения равны дефолтам таблицы; второй restore_all() -> 0 записей.
  5) grep -rn "WriteProcessMemory\|process_vm_writev\|WriteMemory(" src/convars — только вызовы
     через IMemoryBackend (реальных syscall-имён в src/convars/** быть не должно).
  6) grep -c "convars::apply_from_config" src/main.cpp -> 1; больше diff-строк в main.cpp нет.
  7) python3 scripts/check_includes.py (если T023 ещё не сделан — проверить вручную по §4.2:404):
     src/convars/** не включает render/, overlay/, features/.
  8) cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j &&
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
Вход:
  - ARCHITECTURE.md:41 (§2) — слои и дерево каталогов: из чего строить список разрешённых рёбер.
  - ARCHITECTURE.md:379 (§4.1) — таблица «директория -> задача»: кто чем владеет.
  - ARCHITECTURE.md:404 (§4.2) — что запрещено пересекать: готовые правила для check_includes.py.
  - ARCHITECTURE.md:146 (§3.3) и ARCHITECTURE.md:584 — запрет литералов смещений >= 100000.
  - ARCHITECTURE.md:479 (§7) — три законные IPC-границы; их скрипты не считают нарушением.
  - ARCHITECTURE.md:571 (§8.1) — платформы: CI собирается на Linux, Windows-исключения не ломают checks.
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
  - .github/workflows/ci.yml — configure+build+ctest+все проверки, без хоста и без GPU-раннера.
  - .github/workflows/contracts.yml — cron "0 17 * * *" (RESEARCH.md:302) + ручной запуск.
  - tests/fixtures/ci/** — фикстуры для --selftest (сломанный include, лишняя константа,
    изменённый version_file), пишутся в build/selftest/.
Контракт (из ARCHITECTURE.md:404):
  Каждый скрипт: exit 0 если нарушений нет, exit 1 если есть; вывод по одной строке на нарушение
  в формате path:line: сообщение; на stderr — ничего постороннего.
  У каждого скрипта флаг --selftest: офлайн, сети не трогает, сам делает фикстуры в
  build/selftest/ и проверяет, что нарушение обнаруживается (exit 1) и не обнаруживается (exit 0).
  redump_offsets.py никогда не правит contracts/** сам: он либо говорит «no change», либо
  exit 0 + список ключей, которые должен пересобрать T011; без хоста на машине -> не падать,
  а напечатать not-established и exit 0 (ARCHITECTURE.md:588 (§8.2)).
Ограничения:
  Python 3 и только стандартная библиотека: модуля yaml нет, YAML-проверки делать построчными
  регулярками; новых зависимостей и pip install в workflow не добавлять.
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
  3) python3 scripts/redump_offsets.py --selftest --offline (фикстура version_file): дифф пуст ->
     «no change» exit 0; дифф непуст -> exit 0 и построчный список изменённых ключей.
  4) Все скрипты на текущем дереве -> exit 0 (redump при отсутствии хоста -> not-established).
  5) python3 scripts/check_host_env.py --selftest -> фикстура с настоящим именем хоста в `src/`
     даёт exit 1 и `path:line`; имя из `engine_vocabulary` (например `sv_cheats`) -> exit 0.
  5) grep -rn "import yaml\|pip install" scripts .github/workflows — пусто (зависимостей нет).
  6) grep -n "cron" .github/workflows/contracts.yml — ровно "0 17 * * *"; ci.yml не содержит
     шагов, которым нужен хост или GPU.
  7) python3 scripts/check_contracts.py contracts/*.json -> exit 0, и scripts/check_contracts.py
     не изменён (это файл T002).
  8) cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j &&
     ctest --test-dir build --output-on-failure — зелёные без хоста.
Не трогать: src/**, include/**, scripts/check_contracts.py, contracts/schemas/**, contracts/** (только чтение).
```

---

## Таблица зависимостей

Столбец «Блокирует» — только прямые зависимые (обратные рёбра к полю `Зависимости:` в блоках;
транзитивных замыканий в таблице нет). «->» читается как «потому что».

| ID | MS | Зависит от | Блокирует (напрямую) | Почему |
|---|---|---|---|---|
| T000 | M0 | — | T001, T002 | Дерево каталогов + CMake с зелёной сборкой: без этого нечего тестировать и некуда линковать цели. |
| T001 | M0 | T000 | 15: T003, T004, T005, T006, T006W, T007, T010, T012-T015, T019, T021-T023 | `core/` — единые типы, математика и монотонное время; иначе каждая задача выдумает свой vec3 и свои часы. |
| T002 | M0 | T000 | T003, T008, T021, T023 | JSON-схемы + `check_contracts.py`: контракты проверяются машинно, а не «на глаз»; валидатор нужен и дампу, и схеме конфига, и CI. |
| T003 | M1 | T001, T002 | T006, T008, T010, T011 | OffsetRegistry держит смещения как данные (ARCHITECTURE.md:146) и включает проверку устаревания сборки. |
| T004 | M0 | T001 | T005, T006, T006W, T019, T022 | Один заголовок `IMemoryBackend` — шов между «реальный процесс» и «файл с дампом»; на нём держатся все модульный тесты M3+. |
| T005 | M0 | T001, T004 | T006, T006W, T015 | `FakeBackend`: без него ни один парсер нельзя проверить без запущенного хоста, GPU и монитора. |
| T006 | M2 | T001, T003, T004, T005 | T006W, T007, T008, T009, T022 | Чтение живого процесса на Windows (`ReadProcessMemory`): точка, где фейка уже не хватает. |
| T006W | M2 | T001, T004, T005, T006 | T015 | Linux-бэкенд `process_vm_readv` повторяет контракт T006, поэтому идёт после него, а не параллельно: иначе разъедутся сигнатуры. |
| T007 | M3 | T001, T006, T021 | 13: T008, T009, T016, T017, T018, T020, T024-T030 | Публичные `HostState`/`Agent` — единственный источник данных для фич; поменяешь типы -> пересобирается весь M5. |
| T008 | M3 | T002, T003, T006, T007, T011 | T009 | Обход entity list и сетевых слотов (RESEARCH.md:127): самая объёмная часть парсера, невозможна без дампа полей. |
| T009 | M3 | T006, T007, T008, T011 | — | Golden-снапшоты закрывают M3 и становятся регресс-гарантом для любой будущей правки парсера. |
| T010 | M1 | T001, T003 | T011 | Скан паттернов по образу процесса: дамперу нужно находить то, чего нет в JSON-контракте (RESEARCH.md:183). |
| T011 | M1 | T003, T010 | 14: T008, T009, T013, T016, T017, T018, T023, T024-T030 | Критический путь всего проекта: без `m_*`- и view-полей (Q1/Q2) метки, W2S, скелет и кулдауны физически невозможны (RESEARCH.md:199). |
| T012 | M4 | T001, T015 | 13: T013, T014, T016, T017, T018, T020, T024-T030 | `FrameDraw` — общий контракт «фича рисует примитивами и не аллоцирует в run()»; размер буфера берётся из замера T015. |
| T013 | M4 | T001, T011, T012 | 11: T014, T016, T017, T018, T024-T030 | `world_to_screen`: без проекции примитив не знает, куда его рисовать; сама проекция чистая, но требует view-полей T011 (RESEARCH.md:214). |
| T014 | M4 | T001, T012, T013 | T016 | Одно окно оверлея + меню по хоткею: без него кадры негде показать, а настройками нельзя управлять вообще. |
| T015 | M2 | T001, T005, T006W, T021 | T012, T019 | Бенчмарк бюджета кадра + SHM-снапшоты: даёт T012 число, под которое проектируется буфер (ARCHITECTURE.md:416). |
| T016 | M4 | T007, T011, T012, T013, T014, T021 | 10: T017, T018, T020, T024-T030 | Метки + `FeatureRegistry` (ARCHITECTURE.md:328): реестр и `feature.hpp` нужны всем фичам M5, поэтому M5 стартует только после T016. |
| T017 | M5 | T007, T011, T012, T013, T016, T021 | — | Индикатор добивания (`last-hit`/`deny`, RESEARCH.md:231): пороги урона — данные; только чтение, рисует, ничего не пишет. |
| T018 | M5 | T007, T011, T012, T013, T016, T021 | — | Таймеры меток/стоянок/скоплений (RESEARCH.md:232): самый ранний победитель — `dwGlobalVars` плюс типы сущностей. |
| T019 | M6 | T001, T004, T015, T021 | T020 | Input-бэкенды: единственная легальная точка отправки клавиш, запись в `CInput` потребовала бы перехвата (RESEARCH.md:346, Q8). |
| T020 | M6 | T007, T012, T016, T019, T021, T022 | — | «Связка» (RESEARCH.md:234) — единственная input-фича класса I, по умолчанию выключена: её законность в стандартном сетевом режиме не подтверждена (Q8). |
| T021 | M2 | T001, T002 | 15: T007, T015, T016, T017, T018, T019, T020, T022, T024-T030 | Конфиг — единственный источник настроек (RESEARCH.md:245): до него любая фича захардкодила бы цвет, порог и задержку. |
| T022 | M6 | T001, T004, T006, T021 | T020 + W-части T027 и T030 | Convar-ы: вся запись в хост в одном месте и только по белому списку (RESEARCH.md:160); до него W-фичи пишут «кто куда». |
| T023 | M6 | T001, T002, T011 | — | CI-проверки рёбер (§4.2), литералов смещений и свежести дампа: поддерживают T003 и T011, сами ничего не блокируют. |
| T024 | M5 | T007, T011, T012, T013, T016, T021 | — | Скелет по парам костей (RESEARCH.md:226): пока Q1 открыта, рисует not-established, а не угаданный скелет. |
| T025 | M5 | T007, T011, T012, T013, T016, T021 | — | Snap lines (RESEARCH.md:227): видимость = рейка по FOW-данным, отсюда зависимость от T011 и T013. |
| T026 | M5 | T007, T011, T012, T013, T016, T021 | — | Fog overlay «кто кого видит» (RESEARCH.md:228): только чтение, но нужны FOW-поля из дампа. |
| T027 | M5 | T007, T011, T012, T013, T016, T021 | — | Range displays (RESEARCH.md:229): R-версия рисуется сама, W-версия через `cv.range_display` фактически ждёт T022. |
| T028 | M5 | T007, T011, T012, T013, T016, T021 | — | Cooldown tracker (RESEARCH.md:230): фича держится на цепочке полей воздействий из дампа, то есть транзитивно на T011. |
| T029 | M5 | T007, T011, T012, T013, T016, T021 | — | Обманки/истинный обзор (RESEARCH.md:233): `m_hPlayer`-деривация плюс модификаторы — тоже поля из T011. |
| T030 | M5 | T007, T011, T012, T013, T016, T021 | — | Алерты и W-политики (RESEARCH.md:236-235): камера и погода пишут convar-ы, значит полезная часть упирается в T022. |

### Как это запускать

- **Волны (внутри одной строки — параллельно, разным исполнителям):** T000 -> T001, T002 ->
  T004, T021 -> T005, T003 -> T006, T010 -> T006W, T011, T007 -> T015, T008 -> T009, T012 ->
  T013 -> T014 -> T016 -> T017, T018, T019, T022, T023, T024-T030 -> T020.
- **Критический путь:** T000 -> T001 -> T003 -> T010 -> T011 -> T013 -> T014 -> T016 -> M5.
  Всё, что ускоряет чтение памяти и дамп полей, ускоряет проект целиком.
- **T011 — главный блокер:** 14 прямых зависимых, включая весь M4 и весь M5. Пока Q1/Q2
  (RESEARCH.md:335) не закрыты, эти задачи рисуют not-established, но не блокируются сборкой.
- **Один исполнитель = одна задача:** параллельно можно запускать только задачи из одной волны,
  у которых не пересекаются поля `Выход:` (ARCHITECTURE.md:379); единственный общий файл —
  `src/main.cpp` с двумя крюками по одной строке (см. «Общие правила» в начале документа).
- **Осознанная инверсия порядка:** T027 и T030 заявлены как read-only (M5), а их W-варианты
  включают только после T022 (M6). Если нужен жёсткий порядок — добавить T022 в `Зависимости:`
  T027 и T030; делать это в уже принятой задаче нельзя, это правка контракта.
- **Вне таблицы:** наводчик (RESEARCH.md:241) — в бэклоге, ID не назначен. Если делать, он
  встаёт волной после T019 и T022, потому что ему нужны и ввод, и whitelist.
- **Арбитр противоречий:** `PRINCIPLES.md` (P1–P11). Решения P4 (арена вместо `std::vector`),
  P5 (guarded `add_subdirectory` из T000), P6 (ключи фич объявляет T021), P7 (Linux-бэкенд в своей
  директории), P8 (golden-фикстуры с `MANIFEST.sha256`) уже внесены в блоки; волн это не меняет.

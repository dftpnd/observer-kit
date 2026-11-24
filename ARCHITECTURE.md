# ARCHITECTURE — как устроен наблюдатель и где проходят границы

Документ обязан читаться исполнителем задачи **без знания хоста и без знания остальных задач**.
Каждый раздел здесь — это контракт: «тебе дали путь, верни тип с такой-то сигнатурой».

Фактическая база (смещения, convar-ы, ограничения защиты клиента) — `RESEARCH.md:1`.
Словарь терминов проекта — `TERMS.md:1`.
Формат данных-границ — `contracts/README.md:1`.
Формат данных-границ — `contracts/README.md:1`.

## 1. Главное решение: модульный монолит, а не микросервисы

Правило проекта: **контракт обязателен, процесс — нет**. Всё, что можно разделить
заголовком и тестом, разделено заголовком и тестом; процесы поднимаются только там, где
разделение обязано быть адресным (чужая память / чужая машина / чужой рантайм).

Критерии выбора и все спорные решения записаны в `PRINCIPLES.md` (P1–P12): при конфликте
между `ARCHITECTURE.md`, `RESEARCH.md` и `TASKS.md` решает он. Слова в тексте и в именах кода —
только из `TERMS.md`; внешние названия остаются цитатами в `бэктиках`.
между `ARCHITECTURE.md`, `RESEARCH.md` и `TASKS.md` решает он.

| Критерий | Модульный монолит (выбрано) | Микросервисы (процессы+IPC на каждый слой) |
|---|---|---|
| Задержка кадр/состояние | один проход по памяти, ноль сериализации | минимум 2 копии снапшота + ring-шеринг на границах |
| Бюджет 120+ Гц (RESEARCH §5) | выдерживается за счёт батчевого чтения | под вопросом, деградация по числу границ |
| Цена ошибки (наблюдатель = один пользователь, один процесс) | нет распределения отказов → нет distributed failure | 8 процессов = 8 способов умереть и 8 watchdog-ей |
| Приёмка «моделью без контекста» | `ctest` в одном билде, фейковый бэкенд в том же процессе | нужен мок IPC, таймауты, flaky-тесты |
| Делегирование задач | модуль = директория = задача | задача = сервис = Docker/агент + контракт-тест |
| Риск для защиты клиента | один внешний процесс, стабильный профиль | лишние окна/хендлы/процессы = новый профиль совпадений |
| Гибкость языка | C++20 везде | Python/Go удобнее только в дампере, он и так отдельный |

Вывод: **модульный монолит на C++20 + жёсткие внутренние контракты (заголовки) + один
тонкий IPC-шов**. Микросервисная топология оправдана ровно в трёх местах — они вынесены
в §7 как три законные IPC-границы; больше процессов не заводим.

Дополнительный аргумент: наш основной источник риска — не «сложность кода», а **патч хоста,
ломающий смещения** (RESEARCH §13 Q1/Q2). Микросервисы этот риск не уменьшают, а
требование «один процесс + pure-функции + фикстуры» уменьшает, потому что вся логика
проверяется офлайн без запущенного хоста.

## 2. Слои и дерево каталогов

```
                        ┌─────────────────────────────────────────────┐
  человек / конфиг ───► │  config + menu (ImGui)                      │  M2 конфиг / M4 меню
                        └───────────────┬─────────────────────────────┘
                                        │ FeatureRegistry (описание фич, keybinds)
                        ┌───────────────▼─────────────────────────────┐
  логика фич ─────────► │  features/*  (vision, last_hit, timers, combos)  │  M4/M5/M6
   pure-функции         │  вход: const HostState&  выход: FrameDraw   │
                        └───────────────┬─────────────────────────────┘
                                        │ FrameDraw (примитивы, не привязан к рендереру)
                        ┌───────────────▼─────────────────────────────┐
  рисунок ────────────► │  overlay (DX11 | Vulkan + ImGui)            │  M4
                        └───────────────▲─────────────────────────────┘
                                        │ HostState (immutable-снимок кадра + validity bitmask)
                        ┌───────────────┴─────────────────────────────┐
  парсинг ────────────► │  state/parse_frame (чистая функция)         │  M3
                        └───────────────▲─────────────────────────────┘
                                        │ RawFrame (пакеты байт + статусы на каждое чтение)
                        ┌───────────────┴─────────────────────────────┐
  чтение памяти ──────► │  memory/typed_reader поверх IMemoryBackend  │  M2
                        └───────────────▲─────────────────────────────┘
                                        │ ReadRequest{module, symbol, chain[]}
                        ┌───────────────┴─────────────────────────────┐
  имена → числа ──────► │  offsets/OffsetRegistry (contracts/*.json)  │  M1
                        └───────────────▲─────────────────────────────┘
                                        │
             ┌──────────────────────────┴──────────────────────────┐
             │  внешний привилегированный процесс: дампер (Rust)   │  единственная «своя» IPC-граница
             └─────────────────────────────────────────────────────┘
```

```
include/ovr/
  core/        types.hpp vec.hpp time.hpp          # POD-типы, zero deps
  input/       input_backend.hpp key.hpp input_source.hpp   # владение — T019 (§3.9)
  memory/      memory_backend.hpp typed_reader.hpp batch.hpp
  state/       host_state.hpp parse_frame.hpp validity.hpp
  render/      frame_draw.hpp world_to_screen.hpp camera.hpp
  overlay/     overlay.hpp draw_backend.hpp
  features/    feature.hpp feature_registry.hpp
  ipc/         snapshot_stream.hpp                 # только §7
src/           <зеркало include/ovr/…>.cpp         # подкаталоги бэкендов создаёт владелец задачи (T006W)
contracts/     offsets.client-<build>.json info.client-<build>.json fields.client-<build>.json schemas/
scripts/       check_contracts.py check_includes.py check_no_hardcoded_offsets.py redump_offsets.py required_symbols.txt
tests/         unit/ fixtures/ golden/             # снапшоты дампера + MANIFEST.sha256 (P8)
```

**Владение факультативными заголовками.** Факультативные файлы — `offsets/offsets.hpp` (T003), `memory/batch.hpp` (T004),
`state/validity.hpp` (T007), `render/camera.hpp` (T013), `overlay/draw_backend.hpp` (T014), `config/schema.hpp` (T021),
`scripts/required_symbols.txt` и сырой вывод дампера `client.dll.json` (оба — T011) — создаёт и удаляет только их владелец.
Если файл задаче не нужен, его в дереве не появляется (P5); новые подкаталоги — тоже забота владельца задачи,
а корневой `CMakeLists.txt` после T000 не редактирует никто (P5). Все четыре скрипта `scripts/` не дублируют друг друга:
валидация контрактов — T002, правила рёбер и литералов и пересъёмка смещений — T023.

Правило зависимостей строгое и проверяется компилятором (включай только соседний снизу слой):

```
core  ←  offsets  ←  memory  ←  state  ←  features  ←  overlay/input/config
                 └────────────── render ┘
```

- `state/` **не имеет права** включать `overlay/` и `windows.h`.
- `features/` **не имеет права** включать `memory/` — только `HostState`.
- `render/world_to_screen.hpp` **не имеет права** включать ни DX, ни Vulkan.
- `core/` не имеет права включать что-либо, кроме `<cstdint>`, `<array>`, `<cmath>`.

## 3. Внутренние контракты (подписи, которые задачи обязаны вернуть)

Все типы — в `namespace ovr`. Никаких exceptions через границу слоя: статус возвращается
struct-ом. Никакой heap-аллокации в горячем пути кадра (ring/arena).

### 3.1 `include/ovr/core/types.hpp` (T001)

```cpp
namespace ovr::core {
struct Vec2 { float x{}, y{}; };
struct Vec3 { float x{}, y{}, z{}; };
struct Mat4x4 { float m[16]{}; };            // row-major, как кладёт Source2
using ModuleId = std::uint8_t;               // индекс в таблице модулей реестра
constexpr std::uint64_t kInvalidAddr = 0;
}
```

### 3.2 `include/ovr/memory/memory_backend.hpp` (T004)

```cpp
namespace ovr::memory {
struct ReadResult { const std::uint8_t* data; std::uint32_t size; bool ok; };
class IMemoryBackend {
public:
  virtual ~IMemoryBackend() = default;
  virtual bool attach(std::uint32_t pid) = 0;
  virtual std::uintptr_t module_base(std::string_view module) const = 0;
  virtual bool read_raw(std::uintptr_t addr, void* out, std::uint32_t size) = 0;
  virtual void read_batch(std::span<const ReadReq> reqs) = 0;      // один syscall на пачку
};
}
```

Реализации: `RpmBackend` (Win, `ReadProcessMemory`), `ProcessVmReadvBackend` (Linux),
`FakeBackend` (тесты, читает из дамп-файлов фикстур). **`FakeBackend` — обязательное
условие тестируемости: он делается раньше, чем живой бэкенд** (T005 раньше T006).

### 3.3 `include/ovr/offsets/offset_registry.hpp` (T003)

```cpp
namespace ovr::offsets {
enum class Deref : std::uint8_t { None, Once, Chain };   // семантика: RESEARCH §5
struct Symbol { std::uintptr_t rva; Deref deref; std::uint16_t stride; };
class OffsetRegistry {
public:
  static OffsetRegistry load_from(const std::string& contracts_dir, int build);
  std::optional<Symbol> lookup(std::string_view name) const;   // "client.dll!dwGameEntitySystem"
  int build() const;    std::string_view sha256() const;
  bool is_stale(int current_build) const;                      // триггер re-dump (риск R3)
};
}
```

Числа приходят **только** из `contracts/*.json`. В `src/` и `include/` запрещены литералы
`>= 100000` — их ловит `scripts/check_no_hardcoded_offsets.py`.

### 3.4 `include/ovr/memory/typed_reader.hpp` (T006)

```cpp
namespace ovr::memory {
struct ChainStep { Symbol sym; };                 // Once => разыменование, Chain => stride
template <class T> struct Typed { T value{}; bool ok{}; };
class TypedReader {
public:
  template <class T> Typed<T> read(const OffsetRegistry&, std::string_view root_symbol,
                                   std::span<const ChainStep> chain = {}) const;
};
}
```

Правило частичного отказа: любая ссылка в цепочке `== 0` или чтение `size != ожидаем` →
`ok == false`, `value` обнулён, **panic-нет**; состояние помечает бит валидности (см. 3.5).

### 3.5 `include/ovr/state/host_state.hpp` + `parse_frame.hpp` (T007/T008)

```cpp
namespace ovr::state {
struct ActorSnapshot { core::Vec3 pos; int team; int health, max_health, mana, max_mana;
                      int level; std::uint32_t flags; bool alive, visible_to_us; };
struct HostState {
  int tick_client, tick_server; double current_time; int map_width, map_height;
  core::Vec3 camera_origin; core::Mat4x4 view_matrix; float fov_deg, aspect, znear, zfar;
  std::span<ActorSnapshot> actors;   // view в FrameArena: ёмкость фиксирована, аллокаций ноль
  std::span<Agent> agents;            // ёмкость = max_clients и kMaxAgents (см. §5, PRINCIPLES P4)
  std::uint64_t validity_bits;      // бит = «это поле в этом кадре не прочитано»
  bool is_background_map; int sign_on_state;
};
// HostState — view, а не владелец: он живёт один кадр и обязан указывать в FrameArena,
// которая переживает кадры между вызовами. Копировать HostState можно, владеть ареной — нет.
HostState parse_frame(const memory::RawFrame&, const offsets::OffsetRegistry&, state::FrameArena&);
}
```

`parse_frame` — **чистая функция**: на входе байты + реестр, на выходе состояние; ни
окон, ни времени, ни `new`. Отсюда — эталонные тесты: зафиксированный `RawFrame` из
фикстуры ⇒ ожидаемый `HostState` (T009).

### 3.6 `include/ovr/render/frame_draw.hpp` (T012)

```cpp
namespace ovr::render {
struct Line   { core::Vec2 a, b; std::uint32_t rgba; float thickness; };
struct Rect   { core::Vec2 min, max; std::uint32_t rgba; float thickness; bool fill; };
struct Circle { core::Vec2 center; float radius; std::uint32_t rgba; float thickness; bool fill; };
struct Poly   { std::span<const core::Vec2> pts; std::uint32_t rgba; float thickness; bool fill; };
struct Text   { core::Vec2 pos; std::uint32_t rgba; float size; std::string_view utf8; };
using DrawItem = std::variant<Line, Rect, Circle, Poly, Text>;
class FrameDraw {
public:
  void line(core::Vec2 a, core::Vec2 b, std::uint32_t rgba, float thickness = 1.f);
  void rect(core::Vec2 mn, core::Vec2 mx, std::uint32_t rgba, float thickness = 1.f, bool fill = false);
  void circle(core::Vec2 c, float r, std::uint32_t rgba, float thickness = 1.f, bool fill = false);
  void poly(std::span<const core::Vec2> pts, std::uint32_t rgba, float thickness = 1.f, bool fill = false);
  void text(core::Vec2 pos, std::string_view utf8, std::uint32_t rgba, float size = 13.f);
  void clear();
  std::span<const DrawItem> items() const;            // порядок = z-order: раньше добавил ⇒ ниже
  void reserve(std::size_t n);                        // единственный аллоцирующий вызов за кадр
};
}
```

`FrameDraw` — **единственный** способ сказать «нарисуй». Фичи не знают ни про DX, ни про
Vulkan, ни про ImGui; оверлей не знает ни про хост, ни про фичи. Тест: вызвал `line()` →
в `items()` один элемент нужного цвета; вызвал `clear()` → пусто. Аллокаций после
`reserve()` нет (проверяется счётным аллокатором в модульный тесте).

### 3.7 `include/ovr/render/world_to_screen.hpp` (T013)

```cpp
namespace ovr::render {
struct View {
  core::Mat4x4 matrix;                 // row-major, из HostState.view_matrix
  core::Vec3 cam_origin;               // из HostState.camera_origin
  float fov_deg{}, aspect{}, znear{}, zfar{};
  bool valid{};                        // false ⇒ любая проекция возвращает false
};
bool world_to_screen(const View&, core::Vec3 world, core::Vec2& out_screen);
bool screen_to_world_ray(const View&, core::Vec2 screen, core::Vec3& out_dir);
}
```

Пиксельный экран — в **пикселях клиентского окна**, начало координат слева сверху.
Функция **чистая и не зависит от рендерера**: только матрица и арифметика. Обязана
отказывать (возвращать `false`), если `valid == false`, если `w <= 0` (точка за камерой)
или если результат вне окна. `RESEARCH.md:183` фиксирует, что в билде 6985
`dwViewMatrix` нет и матрица придёт из цепочки `dwViewRender` (задача T011); поэтому
контракт публикуется **сейчас**, а тесты пишутся на синтетических `View`
(`tests/unit/test_world_to_screen.cpp`, фикстуры T013): единичная матрица ⇒ точка в центре
экрана, точка за камерой ⇒ `false`, симметрия относительно оси камеры ⇒ зеркальные
пиксели. Живая матрица подставляется позже, тесты от этого не меняются.

### 3.8 `include/ovr/overlay/overlay.hpp` (T014)

```cpp
namespace ovr::overlay {
struct OverlayDesc { std::uint32_t width, height; bool topmost, transparent;
                     std::string_view title; };
class IOverlay {
public:
  virtual ~IOverlay() = default;
  virtual bool init(const OverlayDesc&) = 0;
  virtual void resize(std::uint32_t w, std::uint32_t h) = 0;
  virtual void present(const render::FrameDraw&) = 0;     // vsync-синхронный, не блокирующий > 1 кадра
  virtual bool next_event(core::Vec2& cursor, bool& pressed) = 0;   // для меню
};
IOverlay* create_directx11();   // Windows
IOverlay* create_vulkan();      // Windows и Linux
}
```

Три жёстких требования из `RESEARCH.md:259`: **одно** окно на весь наблюдатель (не по окну на фичу),
`topmost` + прозрачный + без рамки поверх хоста, и **никакого tool-window мигания и смены
фокуса** при появлении/скрытии. Рендер — DX11 или Vulkan (GDI запрещено). Горячая клавиша
меню (`menu_hotkey`) обрабатывается без ввода фокуса на окно оверлея.

### 3.9 `include/ovr/input/input_backend.hpp` (T019)

```cpp
namespace ovr::input {
enum class Key : std::uint16_t { /* VK-коды / Linux codepoints; MOUSE5 = 5 */ };
class IInputBackend {
public:
  virtual ~IInputBackend() = default;
  virtual bool press(Key) = 0;
  virtual bool release(Key) = 0;
  virtual bool is_down(Key) const = 0;
};
IInputBackend* create_sendinput();   // Windows
IInputBackend* create_uinput();      // Linux
}
```

Отправка ввода — **только** `SendInput` (Win) / `uinput` (Linux): никакой записи в
`CInput` чужого процесса (это был бы перехват, `RESEARCH.md:259`). Бэкенд принимает только
клавиши из белого списка задачи T020; попытка отправить клавишу вне списка обязана вернуть
`false` и не должна ничего отправлять. Параметры «связки» берутся из конфига
(`target_lock_key: MOUSE5`, `cast_delay_ms: 15`), а не зашиты в код.

### 3.10 `include/ovr/config/config.hpp` (T021)

```cpp
namespace ovr::config {
struct Config { /* поля = 1:1 с ключами contracts-схемы config.schema.json */ 
  bool overlay_enabled{}; Key menu_hotkey{};
  /* combo / last_hit_helper / vision_tracker / camera — см. RESEARCH §8 */ };
bool load(const std::filesystem::path&, Config& out, std::string& error);
bool save(const std::filesystem::path&, const Config&);
bool validate(const Config&, std::string& error);   // диапазоны, а не «красивость»
}
```

Схема — `config.schema.json` (T021), ключи совпадают с образцом `config.json` из
`RESEARCH.md:218` (`overlay_enabled`, `menu_hotkey`, `combo.*`, `last_hit_helper.*`,
`vision_tracker.*`, `camera.*`). Правила: неизвестный ключ ⇒ ошибка загрузки, а не молчаливое
игнорирование; сбой парсинга ⇒ используются дефолты, наблюдатель не падает; hot-reload по
изменению файла, но **не** посреди кадра (перезагрузка между кадрами).
`camera.custom_distance` валидируется диапазоном 900–2000, `cast_delay_ms` — 0–200,
`cl_weather` — 0–10 (см. риск R8).

### 3.11 `include/ovr/features/feature.hpp` (T016…T022)

```cpp
namespace ovr::features {
class IFeature {
public:
  virtual ~IFeature() = default;
  virtual std::string_view name() const = 0;          // ключ в config.json
  virtual void run(const state::HostState&, const config::Config&,
                   render::FrameDraw&) = 0;           // только чтение состояния
};
class FeatureRegistry {
public:
  void add(std::unique_ptr<IFeature>);
  void run_all(const state::HostState&, const config::Config&, render::FrameDraw&);
  std::span<IFeature* const> features() const;
};
}
```

Фича — это **одна** директория и **один** класс, наследующий `IFeature`. Фича не имеет
права: читать память, хранить указатели на объекты хоста между кадрами, аллоцировать в
`run()` (пишет в `FrameDraw`, у которого сделан `reserve()`), менять convar-ы (для этого
есть отдельный setter T022, вызываемый из `main`, а не из фичи). Новая фича = новый
подкаталог `src/features/<имя>/` + один модульный тест на фикстурах; реестр регистрирует её
одной строкой в `main.cpp`.

---

## 4. Границы модулей: одна задача = одна директория

Это тот раздел, ради которого вся остальная документация написана. Модель без контекста
не должна «понимать» наблюдатель целиком — она должна уметь открыть **одну** директорию, прочитать
в ней 2–3 файла и написать 3-й, не трогая ничего вокруг. Поэтому правило жёсткое:

> **Одна задача = одна директория (плюс её тесты в `tests/unit/`). Две задачи не имеют
> права менять один и тот же файл.**

Отсюда следствия, которые исполнитель обязан соблюсти:

- Задача создаёт/правит файлы **только** внутри своей директории и своего теста. Если для
  реализации нужен чужой заголовок — он уже существует (см. §3) и читается как данность;
  править его нельзя, даже если «очевидно лучше».
- Если контракта из §3 не хватает — задача **не** додумывает сигнатуру, а возвращает
  результат с пометкой «контракт неполон» и предлагает минимальную правку отдельным
  комментарием в PR. Правку контракта делает ревьюер, а не исполнитель.
- Никаких «общих» файлов-помоек вида `utils.hpp`, `helpers.cpp`, `common.hpp`: общий код
  живёт в `core/` и создаётся задачей T001, всё остальное — внутри своей границы.
- Направление `#include` проверяется скриптом (`scripts/check_includes.py`, T023), а не
  на глаз: список разрешённых рёбер графа — тот, что в `ARCHITECTURE.md:41` (§2).

### 4.1 Директория → задача

| Директория / файл | Задача | Что ещё трогает |
| --- | --- | --- |
| `include/ovr/core/`, `tests/unit/test_core.cpp` | T001 | ничего |
| `contracts/schemas/offsets.schema.json`, `scripts/check_contracts.py` | T002 | `contracts/README.md` (только раздел про схемы) |
| `include/ovr/offsets/`, `src/offsets/`, `tests/unit/test_offset_registry.cpp` | T003 | `tests/fixtures/offsets/*.json` (читает) |
| `include/ovr/memory/memory_backend.hpp` | T004 | ничего (только интерфейс) |
| `src/memory/fake_backend/`, `tests/unit/test_fake_backend.cpp` | T005 | фикстуры дампов (читает) |
| `src/memory/rpm_backend/`, `src/memory/vm_readv_backend/`, `tests/unit/test_typed_reader.cpp` | T006, T006W | у каждой задачи своя директория и свой `CMakeLists.txt` (P7) |
| `include/ovr/state/`, `src/state/`, `tests/unit/test_state.cpp` | T007 | ничего |
| `src/state/parse_frame/`, `tests/unit/test_parse_frame.cpp`, `tests/golden/` | T008, T009 | ничего |
| `src/offsets/sigscan/`, `tests/unit/test_sigscan.cpp` | T010 | ничего |
| `tools/dumper/` (обёртка внешнего дампера + нормализация JSON), `contracts/fields.client-<build>.json` | T011 | **блокер** для M3/M4; сам дампер — внешний (P2) |
| `include/ovr/render/`, `src/render/`, `tests/unit/test_render.cpp` | T012, T013 | ничего |
| `include/ovr/overlay/`, `src/overlay/` | T014 | `third_party/` (подмодуль ImGui) |
| `src/features/vision/`, `tests/unit/test_vision.cpp` | T016 | `include/ovr/state/` (читает) |
| `src/features/last_hit/`, `tests/unit/test_last_hit.cpp` | T017 | — |
| `src/features/timers/`, `tests/unit/test_timers.cpp` | T018 | — |
| `include/ovr/input/`, `src/input/`, `src/features/combo/` | T019, T020 | — |
| `include/ovr/config/`, `src/config/`, `config.schema.json` | T021 | — |
| `src/convars/`, `tests/unit/test_convar_whitelist.cpp` | T022 | `RESEARCH.md` §6 (читает) |
| `scripts/` (кроме `check_contracts.py` — это T002), `.github/workflows/` | T023 | — |
| `src/features/<остальные фичи>/` | T024…T030 | по одной директории на фичу |

### 4.2 Что запрещено пересекать

- `state/` не знает про `overlay/`, `render/`, `windows.h` — иначе парсер нельзя тестировать
  офлайн, а это половина ценности всей схемы.
- `features/` не знает про `memory/` — фича ест готовый `HostState`. Хочет что-то новое из
  памяти → заявляет поле в `state/`, и это становится отдельная задача (T007/T008), а не
  `#include "memory/typed_reader.hpp"` внутри фичи меток.
- `render/world_to_screen.hpp` не знает про DirectX/Vulkan — иначе тест матриц требует GPU.
- `contracts/*.json` — вообще не источник кода, а данные; их правит только ре-дампер (T011).

---

## 5. Бюджет кадра: 120 Гц и выше

Хост рендерится с максимальной частотой кадров (240 Гц на железе пользователей), и
оверлей обязан не быть тем, что этот кадр портит. Это не «оптимизация потом», а контракт:
исполнитель задачи пишет код сразу в рамках бюджета, потому что переделывать цикл чтения
дорого.

Целевые цифры (проверяются тестом-бенчмарком T015, а не ощущениями):

| Слой | Бюджет на кадр | Как выполняется |
| --- | --- | --- |
| чтение памяти | ≤ 0.6 мс | один `read_batch()` на кадр, а не 400 `read()` подряд |
| парсинг кадра | ≤ 0.4 мс | чистая функция поверх уже прочитанных байтов, без аллокаций |
| фичи | ≤ 0.3 мс | только арифметика над `HostState`, без обращений к памяти |
| отрисовка | ≤ 0.5 мс | `FrameDraw::reserve()` до начала кадра, дальше только запись в буфер |
| **итого** | **≤ 1.8 мс** | ≈ 550 Гц запас, на 120 Гц остаётся 6× |

Правила, из которых этот бюджет следует:

- **Одна пачка чтения за кадр.** `IMemoryBackend::read_batch()` принимает все запросы
  (entity list, `game rules`, локальный оператор, сетевые слоты) и делает один syscall на Windows
  (`ReadProcessMemory` по возможности слитно) и один на Linux (`process_vm_readv` с iovec).
  400 отдельных вызовов = 400 context switches, и на этом наблюдатель теряет 3–5 мс, что на 240 Гц
  выглядит как фризы.
- **`serverTickCount` читается один раз за кадр.** В build 6985 `deltaTick` и
  `serverTickCount` — один и тот же адрес `588` (`RESEARCH.md:127`), так что «прочитать
  дельту и ещё раз серверный тик» — это две транзакции за одно и то же число, и они могут
  разъехаться между кадрами.
- **Шаг сетевого слота 24 байта**, обход идёт до highest used index, а `entityCount` ≠
  highest index (`RESEARCH.md:127`). Ошибка здесь даёт не «чуть меньше инфы», а кривые метки,
  рисующий мусорные сущности через кадр.
- **Никаких аллокаций в горячем пути.** Ring-буфер под `FrameDraw` и arena под `RawFrame`
  аллоцируются один раз при старте; `parse_frame` не выделяет память вообще — пишет в
  `state::FrameArena` фиксированной ёмкости (`max_clients` из `dwNetworkGameClient_maxClients`
  плюс константа максимум агентов), а `HostState` возвращает в неё `std::span`-ы (PRINCIPLES P4).
  Переполнение арены — не рост буфера, а сброшенные биты валидности: лучше не досчитаться
  одного агента на кадре, чем аллоцировать посреди кадра (P4).
- **Парсинг ≠ рендер.** `parse_frame()` вызывается ровно один раз за кадр и складывает
  результат в слот снапшота; сколько бы окон/фич ни рисовало, память читается один раз.
- **Валидность важнее свежести.** Если любой из запросов чтения провалился — рисуем прошлый кадр и
  поднимаем бит в `validity_bits`, а не рисуем нули (нуль по адресу агента = актор в левом
  нижнем углу экрана, и пользователь думает, что наблюдатель сломался).

---

## 6. Риски и что с ними делать заранее

| ID | Риск | Вероятность | Митигация, зашитая в архитектуру |
| --- | --- | --- | --- |
| R1 | «Trampoline/inline hooking» помечен источником как **DETECTED** (`RESEARCH.md:259`) | высокий | Никаких перехватов, правок кода и подсадки модулей в M0–M7. Только внешнее чтение + легальный ввод. Встроенный вариант — отдельный флаг сборки, не ветка кода. |
| R2 | Запись в память хоста поднимает флаг | высокий | Пишем **только** convar-ы из whitelist (`RESEARCH.md:160`) штатным механизмом, и только если пользователь явно включил фичу. Всё остальное — read-only. |
| R3 | Патч хоста перемонтировал `client.dll`, смещения устарели | почти 100% | `OffsetRegistry::is_stale()` по `ClientVersion` из `version_file` (`RESEARCH.md:302`); CI-задача T023 сравнивает версию и поднимает задачу «смещения устарели»; новый патч = **новый** JSON, а не правка старого. |
| R4 | В 6985 **нет** `dwViewMatrix` (уехал в `removed_in_6985`) | подтверждено | Цепочка через `client.dll!dwViewRender` → view-объект → матрица+FOV/near/far. Пока поля не добыты, метки/rings/добивание **не** принимаются: T011 стоит в критическом пути перед T013 (`RESEARCH.md:183`). |
| R5 | Семантика FOW (per-team grid или сущность) не установлена (Q3) | средний | Знание о тумане спрятано за `src/features/vision/vision_model.hpp`; если гипотеза не подтвердится, меняется один файл, а не 8 фич. |
| R6 | Manual map ломает выгрузку и требует `/Zc:threadSafeInit-` | средний | Не делаем manual map вообще: внешнему режиму он не нужен, а риск совпадения добавляет. |
| R7 | Flaky-тесты из-за межпроцессного взаимодействия | средний | Поэтому процесс один (§1): `ctest` гоняет pure-функции на фикстурах и не требует запущенного хоста. |
| R8 | `cl_weather` противоречив: 9 имён на один диапазон (Q6) | низкий | Валидируем только диапазон 0–10, конкретное имя не считаем фактом; фича погоды помечена experimental. |
| R9 | Законность `SendInput` в стандартном сетевом режиме не подтверждена (Q8) | средний | Все фичи ввода по умолчанию **выключены** в конфиге, включаются явным переключателем; T020 формулирует критерий «можно ли это доказать эмпирически». |
| R10 | Доверие к `deepseek`-выводам и сторонним спискам смещений | средний | Методология доказательств из `RESEARCH.md:100`: поле принимается только при подтверждении из ≥2 независимых источников либо из собственного дампа. |
| R11 | Смещения и фикстуры на Linux приходят только из Windows-прогона: native ELF-скана в дампере нет (`RESEARCH.md:346`, Q7) | высокий | P8: JSON и golden-фикстуры снимаются одним прогоном на Windows и коммитятся с `MANIFEST.sha256`; Linux-CI их только проигрывает без хоста; свой ELF-путь — отдельная задача-исследование вне M0–M7 |

---

## 7. Где процесс всё-таки два: три законные IPC-границы

«Модульный монолит» не означает «ровно один процесс всегда». Процессов ровно **один в
рантайме наблюдателя**; но есть три места, где отдельный процесс оправдан, и все три уже учтены в
дереве каталогов. Больше процессов заводить нельзя — каждая лишняя граница это flaky-тесты
и задержка, которых §5 обещал не иметь.

### 7.1 Дампер смещений → JSON-файл (не сокет)

Дампер — привилегированный, запускается редко (раз в патч), работает 20 секунд и уходит.
Контракт между ним и наблюдателем — **файл** `contracts/offsets.<module>-<build>.json` плюс
`contracts/info.<module>-<build>.json` (`contracts/README.md:1`). Никакого сокета, потому что
нет требования к задержке и нет требования к одновременности: наблюдатель перечитывает файл на старте
и при срабатывании `is_stale()`.

Что это даёт делегированию: исполнитель задачи T011 работает только с дампером и схемами,
ему не нужен остальной код; исполнитель T003 работает только с JSON и реестром, ему не нужен
дампер. Файл — и есть контракт, его можно приложить к задаче как фикстуру.

### 7.2 Стриминг состояния на вторую машину / DMA-ридер

Единственная граница с требованием к задержке. Нужна, когда снижение риска делается через
`pcileech`/DMA (RESEARCH §9): память читает одна машина, а рисует другая.

```cpp
namespace ovr::ipc {
struct SnapshotHeader {
  std::uint32_t magic;           // 'OVRS'
  std::uint16_t schema_version;  // эволюционирует вместе с state::HostState
  std::uint16_t header_size;
  std::uint64_t seq;             // монотонный, приёмник отслеживает пропуски
  std::uint64_t tick_client;
  std::uint64_t tick_server;
  std::uint32_t payload_size;
  std::uint32_t checksum;        // crc32 по payload
};
// ring из 4 слотов, single-producer / single-consumer, без мьютексов
class SnapshotStream {
public:
  bool publish(const state::HostState&);
  bool try_consume(state::HostState& out);   // false = новый снапшот ещё не готов
};
std::unique_ptr<SnapshotStream> create_shm_ring(std::string_view name);
}
```

Правила: один продюсер, один консьюмер, ring из 4 слотов, `seq` для отслеживания пропусков,
приёмник при пропуске или checksum-ошибке рисует прошлый снапшот (см. §5 «валидность важнее
свежести»). Транспорт — POSIX shm; для сценария «вторая машина через Tor» (RESEARCH §12)
поверх того же `SnapshotStream` вешается канал-адаптер, **формат снапшота не меняется**.
Payload — плотная копия: `HostState` — это view поверх `FrameArena` (P4), поэтому `publish()` пишет
в слот POD-поля заголовка, а содержимое span-ов (`actors`, `agents`, …) идёт сразу после них;
`try_consume()` восстанавливает span-ы в собственный arena-буфер, аллокаций после прогрева ноль (T015).
В M0–M7 этот транспорт существует, но по умолчанию не включён: сборка одного процесса от него не зависит.

### 7.3 Lua-слой фич как песочница

Фичи — самый изменчивый слой, и самая частая задача «добавь новую хреновину». Поэтому API
фичи (§3.11) спроектирован так, чтобы его можно было продублировать в Lua: `run(state, config,
draw)` без доступа к памяти.Lua-скрипт — это не отдельный процесс, а способ не перекомпилировать
C++ ради одной фичи. Делается это в M7+ (T030) и не блокирует ничего: если Lua не нужен,
реестр просто содержит C++-фичи.

---

## 8. Как это принимается: тестируемость важнее «красоты»

Приёмка любой задачи из `TASKS.md` — это не «код похож на правду», а две команды:

```
cmake --build build && ctest --test-dir build --output-on-failure
```

Отсюда следует главная инженерная ставка всего проекта: **90% тестов не требуют запущенного
хоста**. Делается это так:

- `FakeBackend` (T005) реализует `IMemoryBackend` поверх дампа-фикстуры из
  `tests/fixtures/dumps/` и отвечает на любые адреса так же, как живой процесс. Живой
  `RpmBackend` (T006) появляется **после** фейка, а не до — иначе ни одна задача M2+ не
  может быть принята на машине без хоста.
- `parse_frame` (T008) — чистая функция `RawFrame + OffsetRegistry → HostState`, поэтому
  проверяется golden-файлами (T009): зафиксированный снапшот кладётся в `tests/golden/`,
  тест сверяет побайтово/по-полям. Прогрел смещения → golden падает, а не «что-то странное
  на экране».
- `world_to_screen` (T013) тестируется на синтетических `View` с известной матрицей
  (тождественная, сдвиг, перспектива 90°): ожидается точное совпадение пикселей. Живой
  матрицы в 6985 ещё нет (§6 R4), и тесты не имеют права на неё ссылаться.
- Все фичи (T016…T030) тестируются на подставных `HostState` — «два актора в 400 агентах,
  один невидимый» и т.п. Фича, для которой нельзя написать такой тест, не принимается.
- Тесты не имеют права: спать «для стабильности», читать `/proc` или реестр Windows, зависеть
  от порядка запуска, требовать GPU/монитора/интернета, оставлять файлы вне `build/`.

### 8.1 Платформы и зависимости

- **Windows-first**, но платформозависимый код живёт строго в трёх местах:
  `IMemoryBackend`-реализация, `Overlay`-бэкенд, `InputBackend`-реализация. Всё остальное
  собирается на обеих ОС, поэтому Linux-сборка — часть приёмки, а не «потом»:
  `mkdir binlinux && cd binlinux && cmake ../ && make`.
- Рендер — DirectX11 **или** Vulkan; ввод — `SendInput` (Win) / `uinput`+SDL3 (Linux);
  чтение — `ReadProcessMemory` / `process_vm_readv`.
- Зависимости внутреннего варианта (`funchook`, `distorm`, Dear ImGui, STB, SDL3) в M0–M7
  не трогаем; наблюдатель тащит из чужих библиотек только ImGui (оверлей) и ничего больше. Дампер смещений —
  внешний готовый Rust-инструмент (`RESEARCH.md:72`, PRINCIPLES P2): T011 пишет только обёртку,
  нормализацию в `contracts/fields.client-<build>.json` и `view_chain` для цепочки матрицы (Q2),
  но не свой парсер Source2 NetworkStrings.
- В `src/` и `include/` запрещены литералы смещений `>= 100000`: число из JSON обязано
  приходить через `OffsetRegistry::lookup()`. Это единственная защита от «заработало на 6985,
  умерло на 6986».

### 8.2 Definition of Done (одинаковый для всех задач)

1. Файлы созданы ровно по списку «выход» из задачи, ничего лишнего.
2. `cmake --build` и `ctest` зелёные на Linux-сборке.
3. Публичные сигнатуры совпадают с §3 буква в букву (проверяется компиляцией чужих тестов).
4. Новый public-заголовок не сломал правило зависимостей из §2.
5. Задача не оставила `TODO`/`FIXME` вместо реализации; неизвестное помечено как
   `not-established` в описании результата, а не захардкожено «красивым» значением.

## 9. Среда хоста (HOST-ENV): идентификация вынесена за репозиторий

Хост внутри рабочей копии никогда не называется своим именем (P12, P13). Всё, что называет его
по имени, — конфигурация, а не знание: реестр настоящих имён лежит в `HOST-ENV.json` **вне git**
и читается по `OVR_HOST_ENV` (по умолчанию `../observer-notes/HOST-ENV.json`).

| Слой | Что содержит | Кто читает |
|---|---|---|
| `HOST-ENV.json` | настоящие имена бинарников, путей, convar-ов, версий, инструментов и источников; ручки `host_bin`, `host_bin_linux`, `version_file`, `patch_current`, `cv.*`, `src-*`, `cache_root` | человек, генератор, `scripts/check_host_env.py` |
| `build/generated/ovr/host_env.hpp` (публичный include `ovr/host_env.hpp`) | `namespace ovr::host_env`: `constexpr std::string_view kTarget, kPatchCurrent, kToolchain, kHostBin, kHostBinLinux, kVersionFile, kCvCameraDistance, kCvRangeDisplay, kCvParticleFow;` + `bool is_available()` | T003 (`version_file`), T006/T011 (`host_bin`), T022 (`cv.*`) |
| `contracts/*.json` | только ручки: `target: "host"`, `patch: "patch_current"`, `toolchain: "src-A"`, `version_source: "version_file"`, `removed_in_<build>: ["cv.particle_fow", …]` | T002, T003 |
| словарь семьи движка (`client.dll`, `engine2.dll`, `sv_cheats`, `r_farz`, `fog_enable`, `cl_weather`) | дословные цитаты: имя хоста они не выдают | все задачи, переводить не нужно |

Правила:

1. **Генерация, а не хардкод.** `scripts/gen_host_env.py` (T000) читает `OVR_HOST_ENV` и пишет
   `host_env.hpp`; корневой `CMakeLists.txt` вызывает его на configure (менять корневой файл
   имеет право только T000 — P5). Литералы имён хоста в `src/**` и `include/**` запрещены наравне
   с литералами смещений (§8.1) и проверяются тем же классом скриптов.
2. **Fail-closed.** Нет файла или нет ручки ⇒ `is_available() == false`, значения — пустые
   `string_view`; задача, которой нужно настоящее имя, отвечает «контракт неполон» (P10), а не
   подставляет «похожее». CI без хоста работает на `tests/fixtures/host_env.sample.json`
   (нейтральные `sample.bin`, `cv_a`, `src-A`): сборка и тесты не зависят от того, что за хост
   стоит у человека или в раннере.
3. **Проверяемость.** `scripts/check_host_env.py` (T023) собирает стоп-лист из значений
   `HOST-ENV.json` (`host.name`, значения `handles`, имена `sources`) и ищет их в файлах
   репозитория **и в сообщениях коммитов**; находка ⇒ `path:line` и exit 1. Значения из
   `engine_vocabulary` нарушением не считаются.
4. **Хост = конфиг.** Новый `HOST-ENV.json` (другая сборка, другая ОС, другой продукт той же
   семьи) не требует правок в коде и документах: ручки остаются те же. Числовые данные
   (`build`, `source_revision`, смещения) идентифицирующими не считаются и живут в контрактах как есть.

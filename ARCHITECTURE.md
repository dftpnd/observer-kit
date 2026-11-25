# ARCHITECTURE — как устроен наблюдатель и где проходят границы

Документ обязан читаться исполнителем задачи **без знания хоста и без знания остальных задач**.
Каждый раздел здесь — это контракт: «тебе дали путь, верни тип с такой-то сигнатурой».

Фактическая база (смещения, convar-ы, ограничения защиты клиента) — `RESEARCH.md:1`.
Словарь терминов проекта — `TERMS.md:1`.
Формат данных-границ — `contracts/README.md:1`.
Правила ведения этих документов и якорей ссылок — §13.

## 1. Главное решение: модульный монолит, а не микросервисы

Правило проекта: **контракт обязателен, процесс — нет**. Всё, что можно разделить
заголовком и тестом, разделено заголовком и тестом; процесы поднимаются только там, где
разделение обязано быть адресным (чужая память / чужая машина / чужой рантайм).

Критерии выбора и все спорные решения записаны в `PRINCIPLES.md` (P1–P14): при конфликте
между `ARCHITECTURE.md`, `RESEARCH.md` и `TASKS.md` решает он. Слова в тексте и в именах кода —
только из `TERMS.md`; внешние названия остаются цитатами в `бэктиках`.
Критерии и порядок их разрешения — `PRINCIPLES.md:7`; новое решение записывается только туда.

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

### 3.1.1 `include/ovr/core/vec.hpp` и `include/ovr/core/time.hpp` (T001)

Продолжение Layer 0: арифметика и единые часы проекта. Реализация — `src/core/vec.cpp`;
заголовков-двойников в дереве нет (DRY).

```cpp
namespace ovr::core {
// vec.hpp
Vec2 operator+(Vec2, Vec2);  Vec2 operator-(Vec2, Vec2);  Vec2 operator*(Vec2, float);
float dot(Vec2, Vec2);  float length_sq(Vec2);  float length(Vec2);
Vec2 normalize(Vec2);   // {0,0,0} => {0,0,0}: ни NaN, ни деления на ноль (E43)
Vec2 lerp(Vec2, Vec2, float t);   // t не конечное => a, NaN наружу не выходит (E43)
Vec3 operator+(Vec3, Vec3);  Vec3 operator-(Vec3, Vec3);  Vec3 operator*(Vec3, float);
float dot(Vec3, Vec3);  float length_sq(Vec3);  float length(Vec3);
Vec3 normalize(Vec3);   // тот же контракт, что у Vec2 (E43)
Mat4x4 operator*(const Mat4x4&, const Mat4x4&);
Vec3 transform_point(const Mat4x4&, Vec3);   // с делением на w; w <= 0 => {0,0,0} (E43)
Vec3 transform_dir(const Mat4x4&, Vec3);     // без деления на w: только направление

// time.hpp
using Clock = std::chrono::steady_clock;
std::uint64_t now_ns();                            // монотонные наносекунды процесса
std::uint32_t ms_to_frames(double ms, double hz);  // вверх; hz <= 0 => 0
}
```

Сна и задержек здесь нет: время только измеряется, решением о паузе занимается цикл кадров (T033).

### 3.2 `include/ovr/memory/memory_backend.hpp` (T004)

```cpp
namespace ovr::memory {
struct ReadResult { const std::uint8_t* data; std::uint32_t size; bool ok; };
struct ReadReq    { std::uintptr_t addr; void* dst; std::uint32_t size; ReadResult* result{}; };  // буфер у вызывающего
class IMemoryBackend {
public:
  virtual ~IMemoryBackend() = default;
  virtual bool attach(std::uint32_t pid) = 0;
  virtual std::uintptr_t module_base(std::string_view module) const = 0;
  virtual bool read_raw(std::uintptr_t addr, void* dst, std::uint32_t size) = 0;
  virtual void read_batch(std::span<const ReadReq> reqs) = 0;      // один syscall на пачку
};
}
```

Реализации: `RpmBackend` (Win, `ReadProcessMemory`), `ProcessVmReadvBackend` (Linux), `FakeBackend` (тесты, читает из дамп-файлов фикстур); запись в чужую память — отдельный интерфейс `IMemoryWriter` (§10.6), его реализуют те же классы, но включают только T022 (convar-ы) и дампер (T011): это единственный канал изменения хоста, всё остальное read-only (риск R2).
**`FakeBackend` — обязательное
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
  bool valid{}; std::uint32_t width{}, height{};  // размеры окна в пикселях
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
using core::Key;   // единый enum клавиш — §10.5 (`core/key.hpp`, владелец T001)
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
`CInput` чужого процесса (это был бы перехват, `RESEARCH.md:259`). Клавиши вне белого списка
в бэкенд не доходят: список отсекает их в `IInputSource` (§10.7), а не в каждом бэкенде.
Параметры «связки» берутся из конфига
(`target_lock_key: MOUSE5`, `cast_delay_ms: 15`), а не зашиты в код.

### 3.10 `include/ovr/config/config.hpp` (T021)

```cpp
namespace ovr::config {
struct Config { /* поля = 1:1 с ключами contracts-схемы config.schema.json */ 
  bool overlay_enabled{}; core::Key menu_hotkey{};   // единый enum клавиш — §10.5 (T001)
  bool markers_enabled{true};        // общий выключатель всех рисующих фич (E16)
  std::uint32_t marker_box_color{};  // индекс палитры из схемы, не RGB-литерал
  float marker_distance_units{1.f};  // во сколько раз world-единица больше шага агента
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


Поля меток — часть контракта конфига, а не фичи: `markers_enabled` гасит все рисующие фичи, и
делает это реестр (T016, §3.11), а не каждая фича по отдельности.
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

Общий выключатель меток (`config::Config::markers_enabled`) проверяет `FeatureRegistry::run_all`:
он не вызывает рисующие фичи вовсе, поэтому фиче не нужно повторять эту проверку (DRY, E16).
Запись в хост — не дело фичи: `IFeature::run` не имеет прав записи, convar-ы пишет setter из T022
из `main`, и только когда фича явно включена (риск R2, E25).

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
| R5 | Семантика сетки видимости (сетка по командам или поле сущности) не установлена (Q3) | средний | Знание о видимости спрятано за `src/features/vision/vision_model.hpp`; если гипотеза не подтвердится, меняется один файл, а не 8 фич. |
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
// ring из kSnapshotSlots слотов (§10.15), single-producer / single-consumer, без мьютексов
class SnapshotStream {           // интерфейс кольца; виртуальный деструктор обязателен:
public:                            // владенец живёт в unique_ptr по базовому указателю (§12.2 L)
  virtual bool publish(const state::HostState&) = 0;   // не блокируется: отсталый слот затирается
  virtual bool try_consume(state::HostState& out) = 0; // false = нового снапшота ещё нет
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

Хост внутри рабочей копии никогда не называется своим именем (P13, P14). Всё, что называет его
по имени, — конфигурация, а не знание: реестр настоящих имён лежит в `HOST-ENV.json` **вне git**
и читается по `OVR_HOST_ENV`: путь задаёт человек или CI, в репозитории он не зашит.

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

---

## 10. Продолжение §3: контракты, которые раньше «закрывала задача»

В `TASKS.md` восемь раз встречалась формулировка «контракт фиксируется этой задачей, ревьюер
перенесёт в §3 комментарием». Пока переноса нет, соседняя задача читает подпись из чужого блока
и копирует её на глаз. Здесь эти подписи зафиксированы один раз; раздел читается как
продолжение §3 — цитировать его надо так же, буква в букву. Формулировку «фиксируется задачей»
в новых блоках писать нельзя (§13.3).

### 10.1 `include/ovr/memory/raw_frame.hpp` (T004)

```cpp
namespace ovr::memory {
struct RawFrame {
  std::span<const std::uint8_t> bytes;   // что вернула пачка read_batch() за кадр
  std::uintptr_t module_base;            // база модуля, от которой считаются RVA
  std::uint64_t frame_seq;               // монотонный номер кадра
  bool complete;                         // false = часть диапазонов пачки не прочитана
};
}
```

Владелец буфера — `FrameSource` (§10.14); `RawFrame` — view, живёт один кадр. `complete == false`
разбор не запрещает: читаем то, что есть, и снимаем биты достоверности (§10.4).
`bytes.empty() == true` допустим в первом кадре и при `is_stale()`; тогда же `complete == false`.

### 10.2 `include/ovr/state/frame_arena.hpp` (T007)

```cpp
namespace ovr::state {
class FrameArena {            // аллоцируется один раз в app (T033), переживает кадры
public:
  void begin_frame();                    // сбрасывает счётчики, память не трогает
  std::span<ActorSnapshot> actors();     // ёмкость kMaxActors, всегда одна и та же
  std::span<Agent> agents();             // ёмкость kMaxAgents + max_clients (§10.15)
  std::uint32_t used_actors() const;
  std::uint32_t used_agents() const;
  bool overflowed() const;               // true => в кадре не хватило мест
};
}
```

Арену копировать нельзя: `HostState` — view в неё (P4). `overflowed() == true` означает, что
разбор остановился раньше и соответствующие биты достоверности сняты; рост буфера посреди кадра
запрещён (§5). Арена не знает ни про память, ни про отрисовку — только слоты и счётчики (§12.2 S).

### 10.3 `state::Agent` (T007) — зафиксирован здесь

```cpp
namespace ovr::state {
struct Agent { std::uint32_t raw_index; core::Vec3 pos; int team;
               int health, max_health; int gold, last_hits, xp;
               std::uint32_t flags; bool alive, is_actor, is_minion, is_guard,
                         is_building, is_beacon; std::uint16_t entity_type; };
}
```

`entity_type` — числовой класс из дампа (`contracts/fields.client-<build>.json`, T011): переводить
его в bool-флаги можно только по таблице из дампа, своих догадок нет (P10). Любое int-поле,
смещения которого в дампе нет, равно `-1`; любой bool в том же случае `false`.

### 10.4 раскладка `validity_bits` — `include/ovr/state/validity.hpp` (T007)

```cpp
namespace ovr::state {
enum class FieldBit : std::uint8_t {
  // 0..15 — кадр, сеть, камера
  frame_tick_client = 0, frame_tick_server, frame_time, frame_map_size,
  frame_background_map, frame_sign_on, camera_origin, view_matrix,
  camera_fov_aspect, camera_clip,
  frame_reserved0, frame_reserved1, frame_reserved2, frame_reserved3,
  frame_reserved4, frame_reserved5,
  // 16..39 — поля актора
  actor_pos = 16, actor_team, actor_health, actor_max_health, actor_mana, actor_max_mana,
  actor_level, actor_flags, actor_alive, actor_visible,
  actor_reserved0, actor_reserved1, actor_reserved2, actor_reserved3, actor_reserved4,
  actor_reserved5, actor_reserved6, actor_reserved7, actor_reserved8, actor_reserved9,
  actor_reserved10, actor_reserved11, actor_reserved12, actor_reserved13,
  // 40..63 — поля агента
  agent_index = 40, agent_pos, agent_team, agent_health, agent_max_health, agent_gold,
  agent_last_hits, agent_xp, agent_flags, agent_alive, agent_type,
  agent_reserved0, agent_reserved1, agent_reserved2, agent_reserved3, agent_reserved4,
  agent_reserved5, agent_reserved6, agent_reserved7, agent_reserved8, agent_reserved9,
  agent_reserved10, agent_reserved11, agent_reserved12
};
bool field_ok(std::uint64_t bits, FieldBit f);   // false => поле в этом кадре не прочитано
}
```

Правила:

- `validity_bits == 0` = «в этом кадре не достоверно ничего»: так у default-сконструированного
  `HostState`, у первого неудачного кадра и у ответа на `is_stale()`.
- бит переходит в «достоверно» при двух условиях сразу: смещение есть в реестре **и** чтение
  удалось. Одно без другого — всё ещё «не знаю».
- `*_reserved*` — свободные номера. Занять резерв имеет право только ревьюер: сначала строка
  здесь, потом код, иначе соседняя задача получит сдвинутый номер бита.
- отдельного набора бит на «все агенты» нет: у агента недостоверность поля показывается
  sentinel-ом (`-1`, §10.3), иначе 400 агентов потребовали бы 400 наборов флагов.

### 10.5 `include/ovr/core/key.hpp` (T001) — один enum клавиш на весь проект

```cpp
namespace ovr::core {
enum class Key : std::uint16_t { unknown = 0 /* VK-коды Windows, коды Linux; MOUSE5 == 5 */ };
std::optional<Key> key_from_name(std::string_view);   // "INSERT" -> Key, "MOUSE5" -> Key
std::string_view key_name(Key);                        // для save() и для меню
}
```

`Config::menu_hotkey` (§3.10), `combo.target_lock_key` и `IInputBackend::press` (§3.9) работают с
одним и тем же типом. Владелец — T001 (Layer 0): конфиг появляется в M2, а `include/ovr/input/`
только в M6. Объявлять `enum class Key` второй раз (в `config.hpp` или в `input/`) запрещено:
два `enum class` с одним именем — два разных типа, и `press(cfg.menu_hotkey)` не соберётся.
Имена клавиш в конфиге — строки (`RESEARCH.md:245`), поэтому таблица «имя ↔ код» нужна и T021,
и T019: она одна, в `core/` (§12.3).

### 10.6 `include/ovr/memory/memory_writer.hpp` (T004) — запись в память хоста

```cpp
namespace ovr::memory {
struct WriteReq    { std::uintptr_t addr; const void* src; std::uint32_t size; };
struct WriteResult { std::uint32_t written; bool ok; };
class IMemoryWriter {
public:
  virtual ~IMemoryWriter() = default;
  virtual bool write_raw(std::uintptr_t addr, const void* src, std::uint32_t size) = 0;
  virtual void write_batch(std::span<const WriteReq>, std::span<WriteResult> out) = 0;
};
}
```

`IMemoryBackend` остаётся read-only, и это проверка риска R2 на уровне типа: у `IFeature::run`
(§3.11) писателя в сигнатуре нет, поэтому фича физически не может изменить хост. Писателя
передаёт явно только T022 (§10.8) и дампер (T011). Реализации — те же классы открыто (никакого
`dynamic_cast` между интерфейсами): `RpmBackend` пишет через `WriteProcessMemory` ровно один раз
и только в `src/memory/rpm_backend/writer.cpp`, `ProcessVmReadvBackend` — через
`process_vm_writev`, `FakeBackend` пишет в свой буфер и считает записи (на этом T022 проверяет
идемпотентность без хоста).

### 10.7 `include/ovr/input/input_source.hpp` (T019) — белый список до обращения к ОС

```cpp
namespace ovr::input {
class IInputSource {            // IInputBackend плюс белый список; фиче нужен именно он
public:
  virtual ~IInputSource() = default;
  virtual bool press(core::Key) = 0;       // false = клавиши нет в списке, бэкенд не тронут
  virtual bool release(core::Key) = 0;
  virtual bool is_down(core::Key) const = 0;
  virtual std::span<const core::Key> whitelist() const = 0;
};
std::unique_ptr<IInputSource> create_input_source(IInputBackend*, std::span<const core::Key>);
}
```

Белый список — данные: список клавиш фиче отдаёт `Config` (T021), а не код задачи. Пустой список
означает «ввод выключен», а не «разрешено всё» (P10). Фича не имеет права звать `IInputBackend`
напрямую: путь в хост один, и он с фильтром (§12.1 Indirection).

### 10.8 `include/ovr/convars/convar_whitelist.hpp` + `src/convars/convars.cpp` (T022)

```cpp
namespace ovr::convars {
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
}
```

Значения берутся только из `Config`; имена convar-ов, в которых написано имя хоста, — ручки из
`ovr/host_env.hpp` (P14). Нет имени в реестре ⇒ не писать и вернуть `false` с именем ключа (P10),
а не подставлять «похожий адрес». `apply_from_config` идемпотентен: повторный вызов с тем же
конфигом не делает ни одной записи; `restore_all` возвращает дефолты таблицы и тоже идемпотентен.
Счётчик записей возвращается тесту и показывается в меню, чтобы «ничего не изменилось» было
чем проверить.

### 10.9 `include/ovr/offsets/sigscan/sigscan.hpp` (T010)

```cpp
namespace ovr::offsets::sigscan {
struct Pattern { std::string_view mnemonic; std::string_view text; };
struct Match { std::uintptr_t address; std::uint32_t size; };
std::vector<Match> scan(std::span<const std::byte> image, std::span<const Pattern> patterns);
std::optional<std::uintptr_t> rip_target(std::span<const std::byte> image, const Match&,
                                         std::size_t disp_offset, std::size_t instruction_length);
}
```

`Pattern.text` — «48 8B 05 ?? ?? ?? ??»: байты через пробел, `??` любой байт, `?X` фиксирует младший ниббл, `X?` фиксирует старший;
разбор толерантный (повторные пробелы, регистр), невалидный шаблон ⇒ 0 совпадений и одна строка
в stderr. `scan()` возвращает все совпадения; `rip_target()` — цель rip-относительного адреса,
`nullopt` при выходе за образ. Вызывать сканер имеет право только дампер (T011): в `src/state/**`,
`src/features/**`, `src/render/**` упоминаний `sigscan` быть не должно.

### 10.10 `include/ovr/features/vision_model.hpp` (T031) — модель видимости

```cpp
namespace ovr::features {
class IVisionModel {
public:
  virtual ~IVisionModel() = default;
  virtual bool is_visible(const state::HostState&, std::uint32_t raw_index, int team) const = 0;
  virtual bool established() const = 0;    // false => «не знаю»: 0 примитивов у фич, которым нужна модель
};
const IVisionModel& null_vision_model();    // реализация по умолчанию
}
```

Вся гипотеза о сетке видимости (Q3, риск R5) спрятана за этим интерфейсом: если она не
подтвердится, меняется один файл T031, а не восемь фич M5. `established() == false` — нормальное
рабочее состояние до закрытия Q3: фичи рисуют 0 примитивов, а не угаданный туман (P10).

### 10.11 CLI дампера `ovr_dumper` (T011)

```text
ovr_dumper [--build <n>] [--out <dir>] [--env <path>] [--dry-run] [--help]
  --build    какой build снимаем (дефолт: host_env::kPatchCurrent)
  --out      каталог результата (дефолт: contracts/); имя файла всегда <вид>.client-<build>.json
  --env      путь к HOST-ENV (P14); нет флага и нет OVR_HOST_ENV => fail-closed (§9)
  --dry-run  собрать и показать, ничего не записывая
  --help     напечатать эти строки и выйти 0
exit 0 = контракт записан либо честно not-established;
exit 1 = внешний дампер не запустился (нет процесса, нет прав, хост не запущен);
exit 2 = битые аргументы или повреждённый входной файл
```

Дампер не читает и не правит `src/**`, `include/**`, `RESEARCH.md`. Запуск живого хоста разрешён
только здесь; в `ctest` его быть не должно (T011 DoD 4).

### 10.12 CLI наблюдателя `ovr` (T033)

```text
ovr [--config <path>] [--env <path>] [--build <n>] [--window <w>x<h>] [--once <n>] [--help]
  --config   путь к config.json (дефолт: ./config.json)
  --env      HOST-ENV (P14); нет файла и нет переменной => host_env::is_available() == false (§9)
  --build    какой снапшот смещений взять (дефолт: по ClientVersion из version_file)
  --window   размер клиентского окна для проекции, пока хост не найден
  --once N   прогнать N кадров и выйти
  --help     напечатать эти строки и выйти 0
exit 0 = штатно; exit 1 = смещения устарели или снапшот не найден (причина — в первой строке
stderr); exit 2 = битый конфиг либо аргументы
```

`--once` закрывает главный разрыв приёмки: без него «CI зелёный» не доказывает, что цикл кадров
вообще собирается. С ним T033 проверяется на фикстурах так же, как остальные задачи, — без хоста,
без GPU и без монитора.

### 10.13 формат эталонной фикстуры (T009)

```json
{ "client_version": 6985, "raw": "frame-6985-play.bin",
  "state": { "<поле из §3.5 или §10.3>": "<значение либо null>" } }
```

`null` означает «поле недостоверно»: у соответствующего бита §10.4 должен быть снят флаг. Имена
полей — ровно имена из §3.5 и §10.3, ни одного нового имени. `.bin` — сырые байты
`RawFrame::bytes` без заголовков: тест не сериализует и не разбирает ничего, кроме дампа.

### 10.14 `include/ovr/state/frame_source.hpp` (T008) — один `read_batch()` за кадр

```cpp
namespace ovr::state {
class FrameSource {
public:
  FrameSource(memory::IMemoryBackend&, const offsets::OffsetRegistry&, FrameArena&);
  bool read_frame();                 // ровно один read_batch() за кадр
  const memory::RawFrame& raw() const;
  std::uint32_t requests() const;    // сколько диапазонов в пачке (бюджет §5)
};
}
```

Здесь живёт план чтения: какие символы нужны, какие размеры, как разложить ответ пачки в
`RawFrame::bytes`. `parse_frame` (§3.5) о плане ничего не знает и остаётся чистой функцией;
`FrameSource` — единственная «нечистая» часть M3 (§12.1 Controller). `is_stale() == true` ⇒
`read_frame()` возвращает `false` и не обращается к памяти вовсе (P9).

### 10.15 Константы ёмкости: кто объявил и кто имеет право менять

| Константа | Значение | Где объявлена | Кто меняет |
|---|---|---|---|
| `kMaxActors` | 10 | `state/frame_arena.hpp` (T007) | ревьюер по дампу |
| `kMaxAgents` | 64 | `state/frame_arena.hpp` (T007) | ревьюер по дампу |
| `max_clients` | из `dwNetworkGameClient_maxClients` (данные) | реестр (T003) | только новый дамп |
| `kEntityScanLimit` | 4096 | `src/state/parse_frame/entity_list.cpp` (T008) | ревьюер (бюджет §5) |
| `kNullSlotRunLimit` | 16 | `src/state/parse_frame/entity_list.cpp` (T008) | ревьюер |
| `kNetSlotStride` | 24 байта (данные дампа, `RESEARCH.md:127`) | `src/state/parse_frame/network.cpp` (T008) | только новый дамп |
| `kDrawReserve` | из замера T015 | `include/ovr/render/frame_draw.hpp` (T012) | ревьюер по бенчмарку |
| `kFrameSlots` | 2 | `src/app/` (T033) | ревьюер |
| `kSnapshotSlots` | 4 | `include/ovr/ipc/snapshot_stream.hpp` (T015) | только вместе с §7.2 |
| `kMaxReadRequests` | 256 | `include/ovr/memory/memory_backend.hpp` (T004) | ревьюер (бюджет §5) |

Это ёмкости и размеры, а не адреса, поэтому запрет на литералы смещений `>= 100000` (§8.1) их не
касается — но каждая объявлена ровно один раз (§12.3). Число сборки в именах файлов и в тестах —
данные, а не константа кода.

### 10.16 `src/app/app.hpp` (T033) — контекст цикла кадров

```cpp
namespace ovr::input { class IInputSource; }        // T019; здесь только forward-объявление
namespace ovr::app {
struct Context {                                     // владелец — app::run; фичи только читают
  memory::IMemoryBackend& backend;
  memory::IMemoryWriter& writer;
  const offsets::OffsetRegistry& offsets;
  config::Config& cfg;
  features::FeatureRegistry& features;
  input::IInputSource* input;                        // nullptr => ввода нет (E21)
};
void register_features(FeatureRegistry&, Context&);  // реализация в src/main.cpp: одна строка на фичу
int run(int argc, char** argv);                      // main.cpp зовёт только это (§10.12)
}
```

Фича получает `IInputSource` конструктором из `register_features`: в сигнатуре `IFeature::run`
(§3.11) ввода нет, поэтому сама фича до ОС дотянуться не может (§12.1 Indirection). Новые поля в
`Context` добавляет только ревьюер: сначала строка здесь, потом код (§13.2). `run` — единственный
владелец цикла кадров; `main.cpp` остаётся «двумя крюками по строке» и `return`-ом (§8.1).

---

## 11. Каталог крайних случаев

Раздел нужен приёмке: если случай из таблицы относится к задаче, её тест обязан его проверять.
«Не знаю, что делать в этом случае» — проваленная приёмка, а не `TODO`. В блоках задач
перечисляются только те строки, которые касаются именно этой задачи; здесь — весь список.

| # | Случай | Кто закрывает | Обязанное поведение |
|---|---|---|---|
| E1 | `HOST-ENV.json` нет либо нужной ручки нет | T000, T023, T033 | `host_env::is_available() == false`, значения пустые, сборка и тесты зелёные; задача отвечает «контракт неполон» (P10) |
| E2 | JSON контракта битый либо ключ не из схемы | T002, T003 | валидатор exit 1; реестр пустой (`build() == 0`, `lookup() == nullopt`), одна строка в stderr, без исключений |
| E3 | фикстуры нет (исполнитель работает без дампа) | T002, T009, T011 | `not-established` и список того, что надо добыть; числа не выдумываются (P10) |
| E4 | сборка хоста не та (`is_stale()`) | T003, T008, T033 | `HostState` с `validity_bits == 0`, span-ы пустые, ни одного обращения к памяти (P9); рисуется прошлый кадр |
| E5 | адрес в цепочке `== 0` на любом шаге | T006, T008 | `ok == false`, `value` обнулён, бит достоверности снят |
| E6 | прочитано меньше байт, чем запрашивали | T004, T006, T008 | частичный отказ по флагам, пачка не уронена; `RawFrame::complete == false` |
| E7 | чтение за концом образа или за границами карты | T005, T006 | `ok == false`, без исключений и без обращения к ОС |
| E8 | пустые слоты подряд, `entity_count` != высший индекс | T008 | обход останавливается после `kNullSlotRunLimit` пустых подряд и не идёт выше `kEntityScanLimit` |
| E9 | арена переполнена (агентов больше ёмкости) | T007, T008 | `overflowed() == true`, лишние не пишутся, биты сняты; аллокаций в кадре ноль (P4) |
| E10 | первый кадр, когда ещё ничего не прочитано | T008, T033 | `HostState` пустой, оверлей не рисует нули в углу экрана (§5) |
| E11 | проекция: `w <= 0`, точка за камерой | T013 | `false`, `out_screen` не тронут; фича пропускает агента — 0 примитивов |
| E12 | `View::valid == false` (view-полей нет в дампе, Q2) | T013, T031 | все проекции `false`, метки не рисуются, в отчёте «не установлено», а не гаданная матрица |
| E13 | `fov_deg <= 0`, `aspect <= 0`, `znear == zfar` | T013 | `false`; деления на ноль и NaN нет |
| E14 | `max_health == 0` | T017 | агент пропущен, бит снят; деления на ноль нет |
| E15 | `map_width/map_height == 0` | T026 | 0 rect; размера клетки достаточно, чтобы не делить на ноль |
| E16 | общий выключатель меток выключен | T016, T033 | `FeatureRegistry::run_all` не вызывает рисующие фичи: 0 примитивов за кадр, фичи не трогали ни память, ни ввод |
| E17 | конфиг: битый JSON, неизвестный ключ, значение вне диапазона | T021 | `load() == false`, все поля равны дефолтам (не частичное состояние), в ошибке — имя ключа |
| E18 | конфиг изменили посреди кадра | T021, T033 | перезагрузка только между кадрами; кадр не видит полу-перезаписанный конфиг |
| E19 | файл конфига удалили после запуска | T021, T033 | дефолты продолжают действовать, наблюдатель не падает и не пишет в лог каждый кадр |
| E20 | клавиша вне белого списка | T019 | `press/release == false`, бэкенд не вызван ни разу |
| E21 | бэкенд недоступен (нет `/dev/uinput`, нет сессии окна) | T019, T033 | фабрика возвращает `nullptr`, наблюдатель работает без ввода, исключений нет |
| E22 | `cast_delay_ms == 0`; два приказа в одном кадре | T020 | пауза считается из `current_time` и тиков; за кадр — не больше одного нажатия |
| E23 | convar-а нет в реестре либо он удалён в этой сборке | T022 | не писать, `false` с именем ключа; `cv.particle_fow` не ищется вообще |
| E24 | повторный `apply_from_config` с тем же конфигом | T022 | ноль записей; повторный `restore_all` — тоже ноль записей |
| E25 | фича выключена, но конфиг просит запись convar-а | T022, T033 | записи нет: канал записи включается только явным переключателем (R2) |
| E26 | приёмник отстал: `seq` или checksum не сошлись | T015 | `try_consume() == false`, `out` не тронут, рисуется прошлый снапшот (§7.2) |
| E27 | второй читатель на том же shm-имени | T015 | не создаётся; тест не требует root и прав superuser |
| E28 | нет GPU, монитора и хоста (CI) | T014, T033, T032 | сборка и тесты зелёные; окно создаётся только в рантайме, в `ctest` его нет |
| E29 | бэкенд чужой платформы в этой сборке | T006, T006W | файл целиком под `#ifdef`; тест пропускается по платформе, а не падает |
| E30 | патч хоста пришёл во время работы | T003, T023, T033 | `is_stale()` останавливает чтение, на экране одна строка «смещения устарели»; новый патч = новый JSON (P3) |
| E31 | алиасы (`dwEntityList == dwGameEntitySystem`) | T002, T003 | `diff` печатает `ALIAS-OK`; это не регрессия |
| E32 | в реестре фич две с одним `name()` | T016 | второй `add()` игнорируется + одна строка в stderr, иначе один ключ конфига читали бы две фичи |
| E33 | `run()` фичи бросил исключение | T016 | `run_all` гасит на границе, остальные фичи вызываются; примитивы этого кадра не «плывут» |
| E34 | пустой `HostState` (0 акторов, 0 агентов) | T016, T017, T018, T024-T030, T031 | 0 примитивов, 0 аллокаций, падений нет |
| E35 | 400 агентов в кадре | T031, T015, T033 | укладывается в бюджет §5; примитивов не больше `kDrawReserve` |
| E36 | у поля фичи в дампе `not-established` | T011, T017, T018, T024-T030, T031 | фича не рисует (0 примитивов) и говорит «не установлено» (P10) |
| E37 | `Text::utf8` указывает на строку короче кадра | T012 | тест на static/literal: время жизни строки — обязанность вызывающего (§3.6) |
| E38 | `reserve(0)`, `clear()` в горячем пути | T012 | 0 аллокаций после прогрева, `items()` пуст |
| E39 | скан: 0 совпадений либо мусорный шаблон | T010 | пустой результат + одна строка в stderr; дампер пишет `not-established` |
| E40 | `rip_target()` считает адрес за образом | T010 | `nullopt`, неопределённого поведения нет |
| E41 | скрипты CI на машине без хоста | T023, T032 | `redump_offsets.py` печатает `not-established` и exit 0; запуски не требуют хоста и GPU |
| E42 | настоящее имя хоста попало в файл или в сообщение коммита | T023, T032 | `check_host_env.py` печатает `path:line` и exit 1 (P14) |
| E43 | ядро: `normalize({0,0,0})`, `w <= 0` в `transform_point`, `lerp` с NaN | T001 | ноль либо `{0,0,0}`; NaN, inf и деления на ноль нет |
| E44 | время хоста стоит или отмоталось (`current_time` не растёт) | T007, T018, T028 | дельта <= 0: таймеры не пересчитываются задом наперёд, остаток не уходит в минус, падения нет |
| E45 | индекс кости вне диапазона либо указатель кости == 0 | T024 | сегмент не рисуется, бит достоверности снят; остальные кости агента не затронуты |
| E46 | цель приказа пропала между кадрами | T020, T025 | линия приказа не рисуется; машина состояний отменяет приказ без обращения к вводу |
| E47 | радиус == 0, отрицательный либо камера в центре кольца | T013, T027 | 0 сегментов либо ограничение минимальным радиусом; NaN и бесконечностей нет |
| E48 | пачка чтения длиннее `kMaxReadRequests` | T004, T005, T006, T006W, T008 | без аллокаций: лишние диапазоны `ok == false`; это находка ревью, а не неопределённое поведение |

---

## 12. Принципы, по которым собран этот код (GRASP · SOLID · DRY · KISS)

Принцип без проверки — пожелание, поэтому у каждой строки есть владелец и машина, которая её
смотрит.

### 12.1 GRASP

| Принцип | Как решено здесь | Чем проверяется |
|---|---|---|
| Эксперт по информации | адреса — у `OffsetRegistry` (T003); мир хоста — у `HostState` (T007); видимость — у `IVisionModel` (T031); разрешённые convar-ы — у `ConvarTable` (T022) | `check_no_hardcoded_offsets.py`, `check_includes.py` (T023) |
| Высокое сцепление | одна задача = одна директория; политика (`*_model`, `*_policy`) отделена от адаптера `IFeature` и от рисования | DoD политики без memory/render/input; §4.2 |
| Низкое сцепление | фичу связывают только `HostState`, `Config` и `FrameDraw`; ОС-зависимость спрятана за интерфейсами T004/T019 | `check_includes.py`; у цели фичи нет `ovr_memory` в линковке |
| Контроллер | `parse_frame` чистая (вход → выход); нечистое вынесено в `FrameSource` (§10.14) и в цикл `app` (T033) | парсер на фикстуре без ОС; эталонные тесты T009 |
| Чистая выдумка | `FrameArena` (§10.2), `RawFrame` (§10.1), `FrameSource` (§10.14) — утилитарные типы с явным владельцем, а не «размазанное» по задачам | матрица владения §14 |
| Полиморфизм вместо веток | поведение платформы = реализация интерфейса (`IMemoryBackend`, `IMemoryWriter`, `IInputSource`, `IVisionModel`, `IOverlay`), а не `switch` | `#ifdef` только в файлах бэкендов (DoD T006, T006W, T019) |
| Защищённость от изменений | новый патч = новый JSON (P3); новая фича = новая директория и одна строка в `main.cpp`; новый ключ = `config.schema.json` (P6) | `is_stale()`; guarded `add_subdirectory` (P5); `check_contracts.py diff` |
| Непрямость | между фичей и ОС — `IInputSource` с белым списком (§10.7); между фичей и хостом — `convars` (§10.8) | тест T019: клавиши нет в списке ⇒ бэкенд не вызван |
| Направление зависимостей | `core ← offsets ← memory ← state ← features ← overlay/input/config`: нестабильное зависит от стабильного, а не наоборот | §2 + `check_includes.py` |

### 12.2 SOLID

| Принцип | Как выполнено | Чем проверяется |
|---|---|---|
| Единая ответственность | `parse_frame` не рисует, `FrameDraw` не читает память, `IFeature` не пишет в хост, `FrameArena` не знает про память | §4.2 + состав линкуемых целей; DoD-тест политики изолированно |
| Открытость/закрытость | новая фича и новый патч не правят общие файлы (§3.11, P3, P5, P6) | DoD «изменены только файлы из Выход»; конфликт возможен только в `src/main.cpp` |
| Подстановка | любой бэкенд памяти и ввода проходит один и тот же контракт-тест: `FakeBackend` (T005) — эталон, живой бэкенд обязан дать те же результаты | `test_typed_reader.cpp` на фейке; тесты бэкендов — тот же набор, кроме E29 |
| Разделение интерфейса | `IMemoryBackend` без записи, писатель — отдельный `IMemoryWriter` (§10.6): запись не «приползает» к фиче случайно | тип `IFeature::run` (§3.11); grep syscall-имён в `src/features/**` |
| Зависимость от абстракций | фичи и разбор зависят от `HostState`, `IMemoryBackend`, `IVisionModel`; конкретные бэкенды создаются только в `app` (T033) | `check_includes.py`: в `src/features/**` нет `memory/`, в `src/state/**` нет бэкендов |

### 12.3 DRY: одно знание — одно место

| Знание | Владелец | Что запрещено |
|---|---|---|
| подпись контракта | §3 и §10 | делать задачу источником подписи: она цитирует раздел (§13.3) |
| enum клавиш и таблица «имя ↔ код» | `core/key.hpp` (T001, §10.5) | объявлять `enum class Key` ещё раз в `config.hpp` или в `input/` |
| биты достоверности | `state/validity.hpp` (T007, §10.4) | вводить свой бит или сдвигать номера без записи здесь |
| таблица convar-ов, диапазоны, дефолты | `convar_whitelist.hpp` (T022) по `RESEARCH.md:160` | дублировать диапазон или дефолт внутри фичи |
| ключи конфига и их границы | `config.schema.json` и `validate()` (T021, P6) | фиче добавлять себе ключ |
| смещения | `contracts/*.json` (T011) | литерал `>= 100000` в коде (§8.1) |
| термины и имена | `TERMS.md:1` | своё слово вместо термина; чужое слово обратно в текст или в имя |
| настоящие имена среды | `HOST-ENV.json` вне копии (P14) | литерал имени хоста в дереве, в коде и в сообщении коммита |
| оговорка «если `check_includes.py` ещё не сделан» | общие правила `TASKS.md:20` | повторять её в каждом блоке задачи |
| бюджет кадра | §5 | «на глаз»; другой порог в другой задаче |

### 12.4 KISS: чем именно режется сложность

- Одна реализация на один способ: своего парсера Source2 нет (P2), второго процесса в рантайме
  нет (P1), общего `utils.hpp` нет (§4).
- Ветки «внутренний доступ» нет как ветки кода: это отдельный флаг сборки (§6 R1).
- Состояние фичи = её поля, созданные до первого кадра: без реестра событий, колбэков и
  синглтонов (§4.2, DoD «0 аллокаций»).
- Отказ = данные: бит достоверности, `false`, `not-established`. Не исключение и не лог.
- Каждая константа названа и объявлена один раз (§10.15); «магических» чисел в математике
  проекции и в политиках фич нет.

---

## 13. Как вести эти документы

### 13.1 Якоря ссылок

Ссылка вида `ARCHITECTURE.md:NNN` или `README.md:NNN` (из `contracts/`) ведёт в **начало
раздела** (`#`, `##`, `###`). Правка, которая сдвигает строки ниже, ломает сразу сотни ссылок,
потому что проверка «влезает ли номер в файл» такой сдвиг не замечает. Правило: либо заменять
строку строкой, либо дописывать в конец файла, либо перенумеровывать все ссылки. Разница между
«ссылка указывает в текст раздела» и «ссылка указывает в начало раздела» — это и есть причина,
по которой документы через месяц начинают врать.

### 13.2 Порядок правки

1. новое решение — в `PRINCIPLES.md` (строка таблицы P);
2. новый термин — в `TERMS.md:1`, новая ручка — в `HOST-ENV.json` (он вне рабочей копии, P14);
3. контракт — в §3, §10 или §14 этого файла;
4. тексты задач — в `TASKS.md`, и только после 1–3.

Обратный порядок — источник расхождения контрактов: задача начинает требовать то, чего в
контракте нет, и исполнитель отвечает «контракт неполон» на уже принятое решение.

### 13.3 Цитаты контрактов в задачах

Задача цитирует подпись из §3/§10 дословно и ссылается на раздел. Формулировка «контракт
фиксируется этой задачей, ревьюер перенесёт в §3» запрещена: до переноса подпись живёт в одном
блоке, а сосед копирует её на глаз. `tools/check_docs.py` сверяет цитаты `IFeature`,
`FeatureRegistry`, `IMemoryBackend`, `OffsetRegistry`, `SnapshotStream` с оригиналом и падает на
расхождении.

### 13.4 Чего в этих документах быть не должно

- настоящих имён среды (стоп-лист `HOST-ENV.json`, P14) и чужих слов вместо наших терминов;
- «примерно», «на глаз», «как правило»: у каждого числа есть владелец (§10.15) и тест (§5);
- `TODO`/`FIXME` вместо реализации и угаданных чисел вместо `not-established` (P10, §8.2);
- второй копии того же знания (§12.3).

---

## 14. Полная матрица владения файлами (продолжение §4.1)

Здесь — файлы, которых в §4.1 нет, и спорные строки §4.1, решённые в пользу одной задачи.
§4.1 остаётся быстрым указателем, при расхождении верна эта таблица. Пересечения проверяет
`tools/check_docs.py`: два блока `Выход:` не имеют права называть один и тот же путь.

| Файл или директория | Владелец | Примечание |
|---|---|---|
| `include/ovr/core/key.hpp`, `src/core/key.cpp` | T001 | §10.5; `config.hpp` и `input_backend.hpp` только включают |
| `include/ovr/memory/raw_frame.hpp` | T004 | §10.1 |
| `include/ovr/memory/memory_writer.hpp` | T004 | §10.6 |
| `src/memory/rpm_backend/writer.cpp` | T006 | `WriteProcessMemory` ровно один раз, только здесь |
| `src/memory/vm_readv_backend/writer.cpp` | T006W | `process_vm_writev` ровно один раз |
| `include/ovr/state/frame_arena.hpp`, `include/ovr/state/validity.hpp` | T007 | §10.2, §10.4 |
| `include/ovr/state/parse_frame/parse_frame.hpp` | T008 | T007 объявляет типы (§3.5), но этот файл не создаёт |
| `include/ovr/state/frame_source.hpp`, `src/state/frame_source/` | T008 | §10.14 |
| `include/ovr/features/feature.hpp`, `feature_registry.hpp`, `src/features/feature_registry.cpp` | T016 | реестр отделён от фичи меток (T031) |
| `src/features/vision/**`, `tests/unit/test_vision.cpp` | T031 | метки и `IVisionModel` (§10.10) |
| `include/ovr/ipc/snapshot_stream.hpp` | T015 | §7.2 |
| `src/app/**`, `tests/unit/test_app.cpp` | T033 | цикл кадров, CLI (§10.12), `--once` |
| `src/main.cpp` | T000 создаёт; крюки: по одной строке `registry.add` у T016…T033, одна строка `convars::apply_from_config` у T022, вызов `app::run` у T033 | единственный общий файл |
| `scripts/gen_host_env.py`, `tests/fixtures/host_env.sample.json` | T000 | §9 |
| `scripts/check_docs.py` | T023 | переезжает из заметок без правок |
| `scripts/check_host_env.py` | T023 | §9, P14 |
| `contracts/evidence/client-<build>/**`, `contracts/MANIFEST.sha256` | T011 | P8 |
| `contracts/fields.client-<build>.json`, `scripts/required_symbols.txt` | T011 | §10.11 |
| `config.schema.json` | T021 | P6 |
| `tests/golden/**` | T009 | P8 |
| `.github/workflows/**` | T032 | запуски CI отдельно от скриптов T023 |
| `third_party/imgui/**` | T014 | подмодуль, больше никто не трогает |

Исключения из правила «один файл — один владелец» ровно два, и оба зафиксированы здесь:
`src/main.cpp` (крюки по одной строке) и `src/app/**` (заглушку создаёт T000, владельцем
становится T033).

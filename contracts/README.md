# Контракты данных (contracts/)

Каталог — единственная граница между «как устроен хост сегодня» и «написанный нами код».
Вся остальная кодовая база обязана обращаться к адресам **только по символьным именам** из этих файлов.

## Что здесь лежит

| Файл | Содержание | Источник |
|---|---|---|
| `offsets.client-<build>.json` | адресные смещения `dw*` по модулям (`client.dll`, `engine2.dll`, `inputsystem.dll`, `soundsystem.dll`, …) | вывод дампера, файл `offsets.json` |
| `info.client-<build>.json` | метаданные снапшота: патч, build number, platform, что удалено в этом билде | наш собственный реестр + `info.json` дампера |
| `client.dll.json` (задача T011) | полевые смещения `m_*` внутри объектов (`m_iHealth`, `m_vecAbsOrigin`, …) | вывод дампера, файл `client.dll.json` |
| `buttons.json` (задача T011) | реестр кнопок/биндов Source2 | вывод дампера, файл `buttons.json` |
| `schemas/offsets.schema.json` (задача T002) | JSON Schema для валидации всех снапшотов | пишем сами |

## Формат снапшота смещений

```json
{ "<module>.dll": { "<SYMBOL_NAME>": <integer>, ... }, ... }
```

Три уровня чисел, их нельзя смешивать:

1. **RVA модуля** — смещение от базы модуля. Пример: `client.dll!dwEntityList = 108484944`.
   Итоговый адрес = `module_base + offset`.
2. **Смещение внутри объекта** — имя содержит суффикс `_` после имени указателя.
   Пример: `dwNetworkGameClient_localPlayer = 248` означает «внутри объекта, на который
   указывает `engine2.dll!dwNetworkGameClient`, по смещению 248 лежит указатель на локального
   оператора». Это **не** RVA.
3. **Статический объект без разыменования** — `dwInputSystem`, `dwSoundSystem`: по
   `module_base + offset` лежит сам объект, никакого лишнего `*` делать не надо.

Правила разыменования — вывод из таблицы семантики `RESEARCH.md` (уровни L1/L2/L3), он живёт в
`src/offsets/offset_registry.cpp` (задача T003): суффикс `_xxx` и статические объекты §5 —
`None`, остальные имена — `Once`; реестр семантику не выбирает, решает слой чтения (§3.4).

## Алиасы, которые не являются ошибкой

- `client.dll!dwEntityList == client.dll!dwGameEntitySystem` (одно и то же место, два имени).
- `engine2.dll!dwNetworkGameClient_deltaTick == dwNetworkGameClient_serverTickCount == 588`
  (одна и та же ячейка памяти, два смысла).
  Слой состояния обязан читать `serverTickCount` один раз за кадр и переиспользовать значение.

## Чего в снапшоте нет (и не будет)

- `dwBuildNumber` — удалён из дампера; версия клиента берётся из
  ручка `version_file` (`version_source: version_file`; настоящее имя файла — в `HOST-ENV.json`).
- `dwViewMatrix` — в билде 6985 статический view matrix не сматчится; матрица видов
  берётся из объекта view (`client.dll!dwViewRender` → цепочка в `client.dll.json`).
- `entityCount` это не «максимальный индекс»: счётчик и максимальный индекс — разные величины,
  обход списка идём до `highest used index`, а не до `entityCount`.

## Версионирование

- Имя файла жёстко привязано к build number клиента: `offsets.client-6985.json`.
- Один закоммиченный снапшот = одна проверенная версия хоста. Новый патч → новый файл,
  старый не правится inplace.
- Загрузка смещений в рантайме: по `ClientVersion`, прочитанному из `version_file` (ручка, P14),
  ищется точное совпадение `offsets.client-<build>.json`; если файла нет — включается
  путь «sigscan + ре-дампа» (см. `ARCHITECTURE.md`, риск R3), а не молчаливое использование
  чужих чисел.
- Ключ сравнения снапшотов — SHA-256 содержимого (как `MANIFEST.sha256` в исходниках-референсах).

## Валидация

`cmake --build` прогоняет `python scripts/check_contracts.py`, который проверяет:

- каждый снапшот валиден по `schemas/offsets.schema.json`;
- все имена из перечня «обязательные символы» (`scripts/required_symbols.txt`) присутствуют;
- ни одно значение не равно `0` и не является строкой;
- алиасы из раздела выше действительно равны.

Запрещено: хардкодить числовые адреса в `.cpp`/`.h`. Проверка `scripts/check_no_hardcoded_offsets.py`
ищет в `src/` и `include/` литералы >= 100000 и роняет сборку.

## Идентичность среды хоста (P14)

`target: "host"`, `patch: "patch_current"`, `toolchain: "src-A"`, `version_source: "version_file"` и
`removed_in_6985: ["cv.particle_fow", …]` — это **ручки**, а не настоящие имена. Настоящие значения
(имя хоста, файл версии, convar-ы, имя инструмента) лежат вне рабочей копии в `HOST-ENV.json` и
разрешаются там же (`TERMS.md`, `ARCHITECTURE.md` §9). Числа `build` и `source_revision` ни хост,
ни жанр не выдают, поэтому остаются как есть.

- Ключи `info.client-<build>.json`: `schema_version, target, platform, arch, renderer, patch, build,
  source_revision, toolchain, exported_formats, version_source, capture_note, removed_in_<build>` —
  ровно эти же имена (snake_case) требует `info.schema.json` (T002).
- `offsets.client-<build>.json`: `schema_version` + карта «модуль → символы». Ни версии, ни sha256
  здесь нет: версия в `info.client-<build>.json`, провенанс в `contracts/MANIFEST.sha256` (P8).
- `scripts/check_host_env.py` (T023) падает, если в файл контракта попало настоящее имя хоста.

## Схемы

Формат обоих снапшотов описан JSON Schema (Draft 2020-12) и проверяется кодом, а не «на глаз».

| Файл | Что описывает |
|---|---|
| `schemas/offsets.schema.json` | `schema_version` + карта «модуль → символы»; смещение — целое 0…0x7FFFFFFF, минимум один модуль |
| `schemas/info.schema.json` | обязательные `schema_version, target, platform, arch, renderer, patch, build, source_revision, toolchain, version_source`; значения-идентичности — ручки либо числа; ключи только snake_case |

Проверка: `python3 scripts/check_contracts.py <файл> [<схема>]` (схема по умолчанию
берётся по форме данных), `diff <файл_a> <файл_b>` для сверки двух снапшотов и
`--manifest` для сверки `MANIFEST.sha256`. Коды выхода: 0 — ок, 1 — нарушение контракта,
2 — файла нет (тогда печатается `not-established` со списком того, что надо добыть).
Зависимостей нет: только stdlib Python.

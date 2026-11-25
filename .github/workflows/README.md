# Локальный запуск CI

Из корня рабочей копии выполни команды в Bash с CMake, компилятором C++20 и Python 3. Команда `python` должна запускать Python 3. Для воспроизведения Windows-сборки с MSVC выставь `CL=/utf-8`. Байтам JSON в рабочей копии нужно совпадать с репозиторием для сверки SHA-256.

| Шаг | Команда |
| --- | --- |
| Configure | `cmake -S . -B build/ci -DCMAKE_BUILD_TYPE=Release -DOVR_BUILD_TESTS=ON -DOVR_HOST_ENV=tests/fixtures/host_env.sample.json` |
| Build | `cmake --build build/ci --config Release --parallel` |
| Test | `ctest --test-dir build/ci -C Release --no-tests=error --output-on-failure` |
| Validate contracts | `for snapshot in contracts/info.client-*.json contracts/offsets.client-*.json; do python scripts/check_contracts.py check "$snapshot" || exit 1; done` |
| Check task dependencies | `python scripts/check_docs.py --selftest` |
| Check includes | `python scripts/check_includes.py` |
| Check offset literals | `python scripts/check_no_hardcoded_offsets.py` |

При падении теста повтори его через `ctest --test-dir build/ci -C Release -R ИМЯ --output-on-failure`. Полный лог лежит в `build/ci/Testing/Temporary/LastTest.log` и в артефакте соответствующей платформы в GitHub Actions. Проверки исходников указывают файл и строку. Кеш сборки привязан к ОС, архитектуре, версии образа раннера и ревизии.

Ежедневный workflow запускает офлайн-мок проверки свежести. Он сообщает `not-established`.

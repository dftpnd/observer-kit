cmake_minimum_required(VERSION 3.22)
project(ovr LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(OVR_BUILD_TESTS "Build ovr unit tests" ON)
set(OVR_HOST_ENV "" CACHE FILEPATH "файл среды хоста; пусто -> заглушка host_env.hpp (E1)")

find_package(Python3 COMPONENTS Interpreter REQUIRED)

# Среда хоста: OVR_HOST_ENV (кэш или переменная окружения), иначе заглушка (PRINCIPLES P14, E1).
set(_ovr_env "")
if(OVR_HOST_ENV)
  set(_ovr_env "${OVR_HOST_ENV}")
elseif(DEFINED ENV{OVR_HOST_ENV})
  set(_ovr_env "$ENV{OVR_HOST_ENV}")
endif()

set(OVR_HOST_ENV_HEADER "${CMAKE_BINARY_DIR}/generated/ovr/host_env.hpp")
set(_ovr_gen "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/scripts/gen_host_env.py"
             "--out" "${OVR_HOST_ENV_HEADER}")
if(_ovr_env)
  list(APPEND _ovr_gen "--env" "${_ovr_env}")
endif()
execute_process(COMMAND ${_ovr_gen} RESULT_VARIABLE _ovr_gen_rc)
if(NOT _ovr_gen_rc EQUAL 0)
  message(FATAL_ERROR "gen_host_env.py не смог написать host_env.hpp")
endif()

add_library(ovr_lib STATIC "${OVR_HOST_ENV_HEADER}")
set_target_properties(ovr_lib PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(ovr_lib PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
  "${CMAKE_BINARY_DIR}/generated")

# Общий include-путь для целей внутри src/: модуль не повторяет эти пути вручную.
add_library(ovr_headers INTERFACE)
target_include_directories(ovr_headers INTERFACE
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
  "${CMAKE_BINARY_DIR}/generated")

add_executable(ovr src/main.cpp)
target_link_libraries(ovr PRIVATE ovr_lib)

macro(ovr_add_dir dir)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${dir}/CMakeLists.txt")
    add_subdirectory(${dir})
  endif()
endmacro()

# Каталог зафиксирован сейчас: новую цель задача описывает внутри своей директории (PRINCIPLES P5).
# Модуль кладёт CMakeLists.txt к себе, объявляет цель вида ovr_<module> и дописывает её в ovr_lib
# строкой target_link_libraries(ovr_lib PUBLIC ovr_<module>). Тест задачи — новый .cpp в
# tests/unit/: он попадает в ctest сам, см. glob в tests/unit/CMakeLists.txt.
ovr_add_dir(src/core)
ovr_add_dir(src/offsets)
ovr_add_dir(src/memory/rpm_backend)
ovr_add_dir(src/memory/vm_readv_backend)
ovr_add_dir(src/memory/fake_backend)
ovr_add_dir(src/state)
ovr_add_dir(src/render)
ovr_add_dir(src/overlay)
ovr_add_dir(src/input)
ovr_add_dir(src/config)
ovr_add_dir(src/convars)
ovr_add_dir(src/ipc)

ovr_add_dir(src/features/vision)
ovr_add_dir(src/features/last_hit)
ovr_add_dir(src/features/timers)
ovr_add_dir(src/features/skeleton)
ovr_add_dir(src/features/snap_lines)
ovr_add_dir(src/features/fow_overlay)
ovr_add_dir(src/features/range_display)
ovr_add_dir(src/features/cooldowns)
ovr_add_dir(src/features/illusions)
ovr_add_dir(src/features/alerts)
ovr_add_dir(src/features/combo)

if(OVR_BUILD_TESTS)
  enable_testing()
  ovr_add_dir(tests/unit)
endif()

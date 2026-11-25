add_library(ovr_memory_fake STATIC fake_backend.cpp dump_loader.cpp)
target_include_directories(ovr_memory_fake PUBLIC "${PROJECT_SOURCE_DIR}/src")
target_link_libraries(ovr_memory_fake PUBLIC ovr_headers)

target_link_libraries(ovr_lib PUBLIC ovr_memory_fake)

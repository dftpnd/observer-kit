add_library(ovr_config STATIC config.cpp)
target_link_libraries(ovr_config PUBLIC ovr_core)
target_link_libraries(ovr_lib PUBLIC ovr_config)

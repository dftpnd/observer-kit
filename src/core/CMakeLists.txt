add_library(ovr_core STATIC key.cpp vec.cpp)
target_link_libraries(ovr_core PUBLIC ovr_headers)

target_link_libraries(ovr_lib PUBLIC ovr_core)

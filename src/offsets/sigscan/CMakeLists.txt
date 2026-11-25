add_library(ovr_sigscan STATIC sigscan.cpp)
target_link_libraries(ovr_sigscan PUBLIC ovr_headers ovr_core)

target_link_libraries(ovr_lib PUBLIC ovr_sigscan)

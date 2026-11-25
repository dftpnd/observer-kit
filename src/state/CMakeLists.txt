add_library(ovr_state STATIC host_state.cpp)
target_link_libraries(ovr_state PUBLIC ovr_core)
target_link_libraries(ovr_lib PUBLIC ovr_state)

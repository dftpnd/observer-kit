add_library(ovr_offline STATIC session.cpp)
target_link_libraries(ovr_offline PUBLIC ovr_features ovr_render ovr_state ovr_offsets)
target_link_libraries(ovr_lib PUBLIC ovr_offline)

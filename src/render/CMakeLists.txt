add_library(ovr_render STATIC frame_draw.cpp world_to_screen.cpp)
target_link_libraries(ovr_render PUBLIC ovr_core)
target_link_libraries(ovr_lib PUBLIC ovr_render)

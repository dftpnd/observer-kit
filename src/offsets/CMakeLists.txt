add_library(ovr_offsets STATIC offset_registry.cpp)
target_link_libraries(ovr_offsets PUBLIC ovr_headers)

target_link_libraries(ovr_lib PUBLIC ovr_offsets)

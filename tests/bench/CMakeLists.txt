add_executable(ovr_bench_frame bench_frame.cpp)
target_link_libraries(ovr_bench_frame PRIVATE ovr_offline)
add_test(NAME bench_frame_smoke COMMAND ovr_bench_frame --smoke)

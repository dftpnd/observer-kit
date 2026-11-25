# Файл принадлежит T000 и больше не правится: новый тест = новый .cpp рядом со smoke.cpp.
file(GLOB _ovr_unit_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
foreach(_src IN LISTS _ovr_unit_sources)
  get_filename_component(_case "${_src}" NAME_WE)
  add_executable("ovr_${_case}" "${_src}")
  target_link_libraries("ovr_${_case}" PRIVATE ovr_lib)
  add_test(NAME "${_case}" COMMAND "ovr_${_case}")
endforeach()

file(REMOVE_RECURSE
  "libovr_lib.a"
  "libovr_lib.pdb"
)

# Per-language clean rules from dependency scanning.
foreach(lang )
  include(CMakeFiles/ovr_lib.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()

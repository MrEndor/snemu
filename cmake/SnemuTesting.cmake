include_guard(GLOBAL)

include(GoogleTest)

# snemu_add_gtest(<name> SOURCES <src>... [LIBRARIES <lib>...])
function(snemu_add_gtest name)
  cmake_parse_arguments(ARG "" "" "SOURCES;LIBRARIES" ${ARGN})

  add_executable(${name} ${ARG_SOURCES})

  target_compile_features(${name} PRIVATE cxx_std_20)
  set_target_properties(${name} PROPERTIES
          CXX_STANDARD_REQUIRED ON
          CXX_EXTENSIONS        OFF)

  target_compile_definitions(${name} PRIVATE SNEMU_UNIT_TEST)
  target_compile_options(${name} PRIVATE --coverage)

  target_link_options(${name} PRIVATE --coverage)
  target_link_libraries(${name} PRIVATE GTest::gtest_main ${ARG_LIBRARIES})

  gtest_discover_tests(${name}
          DISCOVERY_MODE PRE_TEST
          PROPERTIES ENVIRONMENT)
endfunction()

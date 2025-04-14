include(GoogleTest)

file(GLOB_RECURSE TINYCORO_TEST_SOURCES "${PROJECT_SOURCE_DIR}/tests/*test.cpp")

add_custom_target(build-tests COMMAND ${CMAKE_CTEST_COMMAND} --show-only)
add_custom_target(check-tests COMMAND ${CMAKE_CTEST_COMMAND} --verbose)

foreach (tinycoro_test_source ${TINYCORO_TEST_SOURCES})
  get_filename_component(tinycoro_test_filename ${tinycoro_test_source} NAME)
  string(REPLACE ".cpp" "" tinycoro_test_name ${tinycoro_test_filename})
  add_executable(${tinycoro_test_name} EXCLUDE_FROM_ALL ${tinycoro_test_source})
  add_dependencies(build-tests ${tinycoro_test_name})
  add_dependencies(check-tests ${tinycoro_test_name})

  gtest_discover_tests(${tinycoro_test_name}
    EXTRA_ARGS
    --gtest_color=auto
    --gtest_output=xml:${CMAKE_BINARY_DIR}/tests/${tinycoro_test_name}.xml
    --gtest_catch_exceptions=0
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tests
    DISCOVERY_TIMEOUT 120
    PROPERTIES
    TIMEOUT 120
  )

  target_link_libraries(${tinycoro_test_name} ${PROJECT_NAME} gtest)
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(${tinycoro_test_name} PRIVATE "-g")
  endif()
  if(ENABLE_COMPILE_OPTIMIZE)
    target_compile_options(${tinycoro_test_name} PUBLIC -O3)
  endif()

  # Set test target properties and dependencies.
  set_target_properties(${tinycoro_test_name}
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
    COMMAND ${tinycoro_test_name}
  )

  string(REPLACE "_test" "" tinycoro_test_command ${tinycoro_test_name})
  add_custom_target(build-${tinycoro_test_command}
    COMMAND echo "build ${tinycoro_test_command} test..."
    DEPENDS ${tinycoro_test_name}
    COMMENT "build ${tinycoro_test_command} tests..."
  )

  add_custom_target(test-${tinycoro_test_command}
    COMMAND $<TARGET_FILE:${tinycoro_test_name}>
    DEPENDS ${tinycoro_test_name}
    COMMENT "Running ${tinycoro_test_command} tests..."
  )

  add_custom_target(memtest-${tinycoro_test_command}
    COMMAND bash ${PROJECT_SOURCE_DIR}/scripts/memcheck.sh ${PROJECT_SOURCE_DIR}/temp/valgrind_output.xml $<TARGET_FILE:${tinycoro_test_name}> ${PROJECT_SOURCE_DIR}/temp
    COMMAND python ${PROJECT_SOURCE_DIR}/scripts/analysis_valgrind.py ${PROJECT_SOURCE_DIR}/temp/valgrind_output.xml
    $<TARGET_FILE:${tinycoro_test_name}>
    DEPENDS ${tinycoro_test_name}
    COMMENT "Running ${tinycoro_test_command} memtests..."
  )

endforeach ()

# special test for lab3

add_executable(lab3 EXCLUDE_FROM_ALL lab3.cpp)
add_dependencies(build-tests lab3)
add_dependencies(check-tests lab3)
target_link_libraries(lab3 ${PROJECT_NAME})
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  target_compile_options(lab3 PRIVATE "-g")
endif()
if(ENABLE_COMPILE_OPTIMIZE)
  target_compile_options(lab3 PUBLIC -O3)
endif()
set_target_properties(${tinycoro_test_name}
  PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
)

add_custom_target(build-lab3
  COMMAND echo "build lab3 test..."
  DEPENDS lab3
  COMMENT "build lab3 tests..."
)

add_custom_target(test-lab3
  COMMAND python ${PROJECT_SOURCE_DIR}/tests/lab3_test.py $<TARGET_FILE:lab3> ${PROJECT_SOURCE_DIR}/third_party/rust_echo_bench/target/release/echo_bench
  DEPENDS lab3
)

add_dependencies(test-lab3 build-benchtools)


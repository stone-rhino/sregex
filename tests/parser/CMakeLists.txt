file(GLOB local_source
  *.h
  *.cc
)

add_executable(parser_tests ${local_source})
target_include_directories(parser_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(parser_tests PRIVATE parser GTest::gtest_main)
gtest_discover_tests(parser_tests)
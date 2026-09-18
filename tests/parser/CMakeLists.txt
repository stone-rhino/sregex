file(GLOB local_source
  *.h
  *.cc
)


add_executable(parser_tests ${local_source})
target_link_libraries(parser_tests GTest::gtest_main)
gtest_discover_tests(parser_tests)
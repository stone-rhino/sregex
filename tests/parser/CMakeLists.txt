file(GLOB local_source
  *.h
  *.cc
)

find_package(antlr4-runtime CONFIG REQUIRED)

add_executable(parser_tests ${local_source})
target_include_directories(parser_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(parser_tests PRIVATE ${CMAKE_BINARY_DIR}/src/parser/libparser.a antlr4_static GTest::gtest_main)
gtest_discover_tests(parser_tests)
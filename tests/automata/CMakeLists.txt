file(GLOB local_source
  *.h
  *.cc
)

find_package(antlr4-runtime CONFIG REQUIRED)

add_executable(automata_tests ${local_source})
target_include_directories(automata_tests PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(automata_tests PRIVATE ${CMAKE_BINARY_DIR}/src/automata/libautomata.a ${CMAKE_BINARY_DIR}/src/parser/libparser.a antlr4_static GTest::gtest_main)
gtest_discover_tests(automata_tests)
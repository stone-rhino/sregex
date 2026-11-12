file(GLOB local_source
  *.h
  *.cc
)

add_executable(automata_tests ${local_source})
target_include_directories(automata_tests PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(automata_tests PRIVATE automata parser GTest::gtest_main)
gtest_discover_tests(automata_tests)
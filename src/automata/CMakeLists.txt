file(GLOB local_source
  *.h
  *.cc
)

add_library(automata ${local_source})
target_include_directories(automata PRIVATE ${CMAKE_SOURCE_DIR})
  
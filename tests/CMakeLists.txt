file(GLOB local_source
  *.h
  *.cc
)

find_package(GTest CONFIG REQUIRED)
include(GoogleTest)

add_subdirectory(automata)
add_subdirectory(parser)
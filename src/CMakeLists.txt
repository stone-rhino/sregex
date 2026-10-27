file(GLOB local_source
  *.h
  *.cc
)

add_subdirectory(automata)
add_subdirectory(parser)

add_library(sregex
  $<TARGET_OBJECTS:automata>
  $<TARGET_OBJECTS:parser>
  ${local_source}
)
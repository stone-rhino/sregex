file(GLOB local_source
  *.h
  *.cc
)

find_package(antlr4-runtime CONFIG REQUIRED)
find_package(Boost REQUIRED COMPONENTS serialization)

add_subdirectory(automata)
add_subdirectory(parser)

add_library(sregex
  $<TARGET_OBJECTS:automata>
  $<TARGET_OBJECTS:parser>
  ${local_source}
)

target_include_directories(sregex
  PUBLIC
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(sregex INTERFACE antlr4_static Boost::serialization)
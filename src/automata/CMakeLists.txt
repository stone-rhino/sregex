file(GLOB local_source
  *.h
  *.cc
)

find_package(Boost REQUIRED COMPONENTS serialization)

add_library(automata ${local_source})
target_include_directories(automata PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(automata PUBLIC Boost::serialization)
  
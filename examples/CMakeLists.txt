find_package(antlr4-runtime CONFIG REQUIRED)

add_executable(streaming_redaction_with_dfa
  streaming_redaction_with_dfa.cc
)

target_include_directories(streaming_redaction_with_dfa PRIVATE
  ${CMAKE_SOURCE_DIR}
)

target_link_libraries(streaming_redaction_with_dfa PRIVATE
  automata
  parser
  antlr4_static
)
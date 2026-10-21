find_package(antlr4-runtime CONFIG REQUIRED)
find_package(benchmark CONFIG REQUIRED)
find_package(pcre2 CONFIG REQUIRED)

add_executable(nfa_pcre_benchmark
  nfa_pcre_benchmark.cc
)

target_include_directories(nfa_pcre_benchmark PRIVATE
  ${CMAKE_SOURCE_DIR}
)

target_link_libraries(nfa_pcre_benchmark PRIVATE
  automata
  parser
  antlr4_static
  benchmark::benchmark
  PCRE2::8BIT
)
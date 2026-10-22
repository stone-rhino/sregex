find_package(antlr4-runtime CONFIG REQUIRED)
find_package(benchmark CONFIG REQUIRED)
find_package(pcre2 CONFIG REQUIRED)

add_executable(sregex_pcre2_benchmark
  sregex_pcre2_benchmark.cc
)

target_include_directories(sregex_pcre2_benchmark PRIVATE
  ${CMAKE_SOURCE_DIR}
)

target_link_libraries(sregex_pcre2_benchmark PRIVATE
  automata
  parser
  antlr4_static
  benchmark::benchmark
  PCRE2::8BIT
)
file(GLOB local_source
  *.h
  *.cc
)

set(generated_source 
  antlr4_gen/PCREBaseVisitor.h
  antlr4_gen/PCRELexer.h
  antlr4_gen/PCREParser.h
  antlr4_gen/PCREVisitor.h
  antlr4_gen/PCREBaseVisitor.cpp
  antlr4_gen/PCRELexer.cpp
  antlr4_gen/PCREParser.cpp
  antlr4_gen/PCREVisitor.cpp
)

add_custom_command(
  OUTPUT ${generated_source}
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/src/parser
  COMMAND java -Xmx500M -cp "${CMAKE_SOURCE_DIR}/tools/antlr-4.13.2-complete.jar:$CLASSPATH" org.antlr.v4.Tool -Dlanguage=Cpp -visitor -no-listener -package Sregex::Parser::Antlr4Gen -o ${CMAKE_BINARY_DIR}/src/parser/antlr4_gen *.g4
  DEPENDS ${CMAKE_SOURCE_DIR}/src/parser/PCRE.g4
)

find_package(antlr4-runtime CONFIG REQUIRED)

add_library(parser ${local_source} ${generated_source})
target_include_directories(parser PRIVATE ${CMAKE_BINARY_DIR}/src/parser)
target_link_libraries(parser PRIVATE antlr4_static)
  
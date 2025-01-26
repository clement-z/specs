#include_directories(${CMAKE_CURRENT_SOURCE_DIR})
#include_directories(${CMAKE_CURRENT_BINARY_DIR})

file(GLOB HH_FILES *.h)
file(GLOB SRC_FILES *.cpp)

find_package(BISON)
find_package(FLEX)

message("hey ${SystemC_CXX_STANDARD}")
message("hey ${SystemC_INCLUDE_DIRS}")

message("hey ${PROJECT_INCLUDE_DIRS}")

set(BISONFLAGS "--warnings -Wall" ) # -Wcex
file(MAKE_DIRECTORY ${PARSER_OUTPUT_DIR})

bison_target(Parser parser.y ${PARSER_OUTPUT_DIR}/parser.cpp COMPILE_FLAGS "${BISONFLAGS}" DEFINES_FILE ${PARSER_OUTPUT_DIR}/parser.h )
flex_target(Scanner parser.l ${PARSER_OUTPUT_DIR}/scanner.cpp  DEFINES_FILE ${PARSER_OUTPUT_DIR}/scanner.h)

add_flex_bison_dependency(Scanner Parser)

add_library(parse
        STATIC
        ${FLEX_Scanner_OUTPUTS}
        ${BISON_Parser_OUTPUTS}
        ${HH_FILES}
        ${SRC_FILES}
)
target_include_directories(parse PRIVATE ${PARSER_OUTPUT_DIR})
target_include_directories(parse PRIVATE ${PARSER_OUTPUT_DIR})
target_include_directories(parse PRIVATE ${SystemC_INCLUDE_DIRS})
target_include_directories(parse SYSTEM PRIVATE /home/czrounba/git/specs/src)
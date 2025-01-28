file(GLOB HH_FILES
        RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
        CONFIGURE_DEPENDS
        "*.h")
file(GLOB SRC_FILES
        RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
        CONFIGURE_DEPENDS
        "*.cpp")

find_package(BISON)
find_package(FLEX)

set(BISONFLAGS "--warnings -Wall" ) # -Wcex
file(MAKE_DIRECTORY ${PARSER_OUTPUT_DIR})

bison_target(Parser parser.y ${PARSER_OUTPUT_DIR}/parser.cpp COMPILE_FLAGS "${BISONFLAGS}" DEFINES_FILE ${PARSER_OUTPUT_DIR}/parser.h )
flex_target(Scanner parser.l ${PARSER_OUTPUT_DIR}/scanner.cpp  DEFINES_FILE ${PARSER_OUTPUT_DIR}/scanner.h)

add_flex_bison_dependency(Scanner Parser)

#target_sources(${PROJECT_NAME} PRIVATE ${SRC_FILES} ${HH_FILES})
#target_sources(${PROJECT_NAME} PRIVATE ${FLEX_Scanner_OUTPUTS} ${BISON_Parser_OUTPUTS})

SET_SOURCE_FILES_PROPERTIES(${FLEX_Scanner_OUTPUTS} ${BISON_Parser_OUTPUTS} GENERATED)

message(STATUS ___ "${BISON_Parser_OUTPUTS}" "${FLEX_Scanner_OUTPUTS}")
add_library(${PROJECT_NAME}_parser OBJECT EXCLUDE_FROM_ALL
        ${SRC_FILES}
        ${HH_FILES}
        ${FLEX_Scanner_OUTPUTS}
        ${BISON_Parser_OUTPUTS}
)

target_link_libraries(${PROJECT_NAME}_parser PRIVATE common)
target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}_parser)


get_cmake_property(_variableNames VARIABLES)
list (SORT _variableNames)
foreach (_variableName "HH_FILES" "SRC_FILES")
    message(STATUS "${_variableName}=${${_variableName}}")
endforeach()
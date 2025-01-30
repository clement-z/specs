cmake_minimum_required(VERSION 3.14)

# Project Name
project(specs LANGUAGES CXX)
option(BUILD_OLD_TB "Build old testbenches" ON)
option(BUILD_TB "Build testbenches" ON)

set(LIB_NAME "spx")

# Specify C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SystemCLanguage CONFIG REQUIRED PATHS "${CMAKE_SOURCE_DIR}/thirdparty/systemc/build/install")
find_package(args CONFIG REQUIRED PATHS "${CMAKE_SOURCE_DIR}/thirdparty/args/build/install")


# Find source files
set(SRC_PATH ${CMAKE_SOURCE_DIR}/src)
file(GLOB_RECURSE SOURCES_BIN CONFIGURE_DEPENDS "${SRC_PATH}/*.cpp")
list(FILTER SOURCES_BIN EXCLUDE REGEX ".*/tb/.*")
list(FILTER SOURCES_BIN EXCLUDE REGEX ".*/parser/.*")
list(FILTER SOURCES_BIN EXCLUDE REGEX ".*/test/.*")

set(SOURCES_LIB ${SOURCES_BIN})
list(FILTER SOURCES_LIB EXCLUDE REGEX ".*/main.cpp")

# Parser directory definitions
set(GENERATED_OUTPUT_DIR "${CMAKE_BINARY_DIR}/generated")
set(PARSER_OUTPUT_DIR "${GENERATED_OUTPUT_DIR}/parser")

# ****************************************************************

add_library(common INTERFACE)

target_compile_options(common INTERFACE -Wall -Wextra)
target_compile_options(common INTERFACE -march=native)
target_compile_options(common INTERFACE -Wfatal-errors)
#target_compile_options(common INTERFACE -v)
#target_link_options(common INTERFACE -v)

target_compile_options(common INTERFACE -iquote ${SRC_PATH})
target_link_libraries(common INTERFACE SystemC::systemc)


# ****************************************************************

# Main binary and library
add_executable(${PROJECT_NAME} ${SOURCES_BIN} ${SOURCES_TB})
add_library(${LIB_NAME} SHARED EXCLUDE_FROM_ALL ${SOURCES_LIB})

# Link to common config
target_link_libraries(${PROJECT_NAME} PRIVATE common)
target_link_libraries(${LIB_NAME} PRIVATE common)

# Link args and math
target_link_libraries(${PROJECT_NAME} PRIVATE taywee::args m)

# *****************************************************************

# Add old tb subdirectory
add_subdirectory(${SRC_PATH}/tb)

# Add unit tests subdirectory
add_subdirectory(${SRC_PATH}/test)

# Add parser subdirectory
add_subdirectory(${SRC_PATH}/parser)

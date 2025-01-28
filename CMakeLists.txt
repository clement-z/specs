cmake_minimum_required(VERSION 3.14)

# Project Name
project(specs LANGUAGES CXX)

# Specify C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include paths
set(SRC_PATH ${CMAKE_SOURCE_DIR}/src)

set(SYSTEMC_INSTALL_ROOT ${CMAKE_SOURCE_DIR}/thirdparty/systemc/sc_install)
set (CMAKE_PREFIX_PATH ${SYSTEMC_INSTALL_ROOT})
find_package(SystemCLanguage CONFIG REQUIRED)
set (CMAKE_PREFIX_PATH ${CMAKE_SOURCE_DIR}/thirdparty/args/build/install)
find_package(args CONFIG REQUIRED)


set (SystemC_INCLUDE_DIRS "${SYSTEMC_INSTALL_ROOT}/include")


option(BUILD_TB "Build testbenches" ON)

# Find source files
file(GLOB_RECURSE SOURCES_BIN CONFIGURE_DEPENDS "${SRC_PATH}/*.cpp")
list(FILTER SOURCES_BIN EXCLUDE REGEX ".*/tb/.*")
list(FILTER SOURCES_BIN EXCLUDE REGEX ".*/parser/.*")

# Main binary
add_executable(${PROJECT_NAME} ${SOURCES_BIN} ${SOURCES_TB})

# Add tb config
add_subdirectory(${SRC_PATH}/tb)

# Add tb config
set(GENERATED_OUTPUT_DIR "${CMAKE_BINARY_DIR}/generated")
set(PARSER_OUTPUT_DIR "${GENERATED_OUTPUT_DIR}/parser")
add_subdirectory(${SRC_PATH}/parser)

add_library(common INTERFACE)

target_compile_options(common INTERFACE -Wall -Wextra)
target_compile_options(common INTERFACE -march=native)
target_compile_options(common INTERFACE -Wfatal-errors)
#target_compile_options(common INTERFACE -v)


#target_include_directories(${PROJECT_NAME}  PRIVATE ${SRC_PATH})
target_compile_options(common INTERFACE -iquote ${SRC_PATH})
target_include_directories(common INTERFACE ${PARSER_OUTPUT_DIR})
target_include_directories(common SYSTEM INTERFACE ${SystemC_INCLUDE_DIRS})

# Add generated sources and headers to the project
target_link_libraries(${PROJECT_NAME} PRIVATE common)
target_link_libraries(${PROJECT_NAME} PRIVATE SystemC::systemc taywee::args m)


get_target_property(ii specs INCLUDE_DIRECTORIES)

#get_cmake_property(_variableNames VARIABLES)
#list (SORT _variableNames)
#foreach (_variableName ${_variableNames})
#    message(STATUS "${_variableName}=${${_variableName}}")
#endforeach()
cmake_minimum_required(VERSION 3.14)

# Project Name
project(specs LANGUAGES CXX)

# Specify C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include paths
set(SRC_PATH src)

set(SYSTEMC_INSTALL_ROOT ${CMAKE_SOURCE_DIR}/thirdparty/systemc/sc_install)
set (CMAKE_PREFIX_PATH ${SYSTEMC_INSTALL_ROOT})
find_package(SystemCLanguage CONFIG REQUIRED)
set (CMAKE_PREFIX_PATH ${CMAKE_SOURCE_DIR}/thirdparty/args/build/install)
find_package(args CONFIG REQUIRED)

set (SystemC_INCLUDE_DIRS "${SYSTEMC_INSTALL_ROOT}/include")
message("${CMAKE_PREFIX_PATH}")
message("${SystemC_CXX_STANDARD}")


#set(SYSTEMC_PATH_ROOT ${CMAKE_SOURCE_DIR}/thirdparty/systemc/sc_install)
#set(SYSTEMC_PATH_LIBS ${SYSTEMC_PATH_ROOT}/lib)
#set(SYSTEMC_PATH_INCLUDE ${SYSTEMC_PATH_ROOT}/include)
#include_directories(${SYSTEMC_PATH_INCLUDE} thirdparty/args)

# Find source files
file(GLOB_RECURSE SOURCES_BIN "${SRC_PATH}/*.cpp")
list(FILTER SOURCES_BIN EXCLUDE REGEX ".*/tb/.*")
#list(REMOVE_ITEM SOURCES_BIN "${SRC_PATH}/main.cpp")
file(GLOB_RECURSE SOURCES_TB "${SRC_PATH}/tb/*.cpp")

# Function to recursively collect directories
function(collect_directories BASE_DIR OUTPUT_VAR)
    set(DIRS "")
    file(GLOB CHILDREN LIST_DIRECTORIES true "${BASE_DIR}/*")
    foreach(CHILD ${CHILDREN})
        if(IS_DIRECTORY "${CHILD}")
            list(APPEND DIRS "${CHILD}")
            collect_directories("${CHILD}" SUBDIRS)
            list(APPEND DIRS ${SUBDIRS})
        endif()
    endforeach()
    set(${OUTPUT_VAR} ${DIRS} PARENT_SCOPE)
endfunction()

# Call the function to collect directories under SRC_PATH
collect_directories("${CMAKE_SOURCE_DIR}/${SRC_PATH}" PROJECT_INCLUDE_DIRS)


# Add to include directories
#include_directories(${PROJECT_INCLUDE_DIRS} ${SRC_PATH})
#include_directories(${SYSTEMC_PATH_INCLUDE})

# Main binary
add_executable(${PROJECT_NAME} ${SOURCES_BIN} ${SOURCES_TB})

set(PARSER_OUTPUT_DIR "${CMAKE_BINARY_DIR}/parser")
add_subdirectory(${SRC_PATH}/parser)

# Print collected directories
message("------------------------- ${PROJECT_INCLUDE_DIRS}")
message("------------------------- ${PARSER_OUTPUT_DIR}")
message("-------- ${SystemC_INCLUDE_DIRS} ---")

target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)
target_compile_options(${PROJECT_NAME} PRIVATE -march=native)
target_compile_options(${PROJECT_NAME} PRIVATE -Wfatal-errors)
#target_compile_options(${PROJECT_NAME} PRIVATE -v)

option(BUILD_TB "Build testbenches" ON)
if(BUILD_TB)
target_compile_definitions(${PROJECT_NAME} PUBLIC "BUILD_TB=1")
endif()

# Compiler and linker options
#set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -L${SYSTEMC_PATH_LIBS} -lsystemc -lm -Wl,-rpath,${SYSTEMC_PATH_LIBS}")


# Add generated sources and headers to the project
#target_link_directories(${PROJECT_NAME} PRIVATE ${SYSTEMC_PATH_LIBS})
target_link_libraries(${PROJECT_NAME} PRIVATE SystemC::systemc taywee::args parse m)
#target_link_options(${PROJECT_NAME} PRIVATE -Wl,-rpath,-${SYSTEMC_PATH_LIBS})

#target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE ${PROJECT_INCLUDE_DIRS} ${SRC_PATH})
target_include_directories(${PROJECT_NAME} PRIVATE ${SRC_PATH})
target_include_directories(${PROJECT_NAME} PRIVATE ${PARSER_OUTPUT_DIR})
target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE ${SystemC_INCLUDE_DIRS})

get_target_property(ii specs INCLUDE_DIRECTORIES)
message(STATUS "${ii}")
message(STATUS "${PROJECT_NAME}")

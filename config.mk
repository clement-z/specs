#### PROJECT SETTINGS ####
# The name of the executable to be created
BIN_NAME = specs
LIB_NAME = libspecs.so

SYSTEMC_PATH_ROOT = $(abspath ./thirdparty/systemc/sc_install)
SYSTEMC_PATH_LIBS = ${SYSTEMC_PATH_ROOT}/lib
SYSTEMC_PATH_INCLUDE = ${SYSTEMC_PATH_ROOT}/include

# Compiler used
CXX = g++
#CXX = clang++
CCACHE =
#CCACHE = ccache

# Path to the source directory, relative to the makefile
SRC_PATH = src

# General compiler flags
CXXFLAGS = -std=c++17 -Wall -Wextra
CXXFLAGS += -O2 -march=native
#CXXFLAGS += -DYYDEBUG=1
CXXFLAGS += -g

# Add additional include paths (SRC_PATH and subdirectories are automatically added)
INCLUDES = -I${SYSTEMC_PATH_INCLUDE} -isystem thirdparty/args

# General linker settings
LDFLAGS += -L${SYSTEMC_PATH_LIBS} -lsystemc -lm
LDFLAGS += -Wl,-rpath -Wl,${SYSTEMC_PATH_LIBS}

# Destination directory
BUILD_PATH = build
BUILD_TB ?= 1
#### END PROJECT SETTINGS ####

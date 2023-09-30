#### PROJECT SETTINGS ####
# The name of the executable to be created
BIN_NAME = specs
LIB_NAME = libspecs.so

# Compiler used
CXX = g++
CCACHE =

BUILD_TB ?= 1

# Path to the source directory, relative to the makefile
SRC_PATH = src

# General compiler flags
CXXFLAGS = -std=c++17 -Wall -Wextra
CXXFLAGS += -O2 -march=native

# Add additional include paths (SRC_PATH and subdirectories are automatically added)
INCLUDES += -isystem thirdparty/args

# General linker settings
LDFLAGS += -lsystemc -lm

# Destination directory
BUILD_PATH = build

#### END PROJECT SETTINGS ####

# This file will fetch the version from git tags and use it in CMake.

# Check if we have Git available
find_package(Git REQUIRED)

# Get the latest tag
execute_process(
        COMMAND git describe --tags --abbrev=0
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_LATEST_TAG
        OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Get the number of commits since the last tag
execute_process(
        COMMAND git rev-list ${GIT_LATEST_TAG}..HEAD --count
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_COMMITS_SINCE_TAG
        OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Get the current commit hash
execute_process(
        COMMAND git rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_COMMIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Determine the version string
if("${GIT_COMMITS_SINCE_TAG}" STREQUAL "0")
    # If this is exactly the tagged commit, just return the tag
    set(GIT_VERSION "${GIT_LATEST_TAG}")
else()
    # Otherwise, append the commit count and hash
    set(GIT_VERSION "${GIT_LATEST_TAG}-${GIT_COMMITS_SINCE_TAG}.g${GIT_COMMIT_HASH}")
endif()

# Parse the version components (major, minor, patch)
string(REGEX MATCH "^v([0-9]+)\\.([0-9]+)\\.([0-9]+)" SEMVER_MATCHES "${GIT_LATEST_TAG}")
if(SEMVER_MATCHES)
    string(REGEX REPLACE "^v([0-9]+)\\..*" "\\1" PROJECT_VERSION_MAJOR "${SEMVER_MATCHES}")
    string(REGEX REPLACE "^v[0-9]+\\.([0-9]+)\\..*" "\\1" PROJECT_VERSION_MINOR "${SEMVER_MATCHES}")
    string(REGEX REPLACE "^v[0-9]+\\.[0-9]+\\.([0-9]+).*" "\\1" PROJECT_VERSION_PATCH "${SEMVER_MATCHES}")
    set(PROJECT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")
endif()

# Set the full version
set(PROJECT_VERSION_FULL "${GIT_VERSION}")
if("${PROJECT_VERSION_FULL}" STREQUAL "")
    set(PROJECT_VERSION_FULL "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")
endif()
message(STATUS "Read version from tags: ${PROJECT_VERSION_FULL} (${PROJECT_VERSION_MAJOR}, ${PROJECT_VERSION_MINOR}, ${PROJECT_VERSION_PATCH})")

# Configure the version.txt file
configure_file(
        ${CMAKE_SOURCE_DIR}/cmake/version.txt.in
        ${CMAKE_BINARY_DIR}/version.txt
        @ONLY
)

# Configure the version.h header file
set(PROJECT_VERSION_INCLUDE_DIR ${CMAKE_BINARY_DIR}/generated/utils)
configure_file(
        ${CMAKE_SOURCE_DIR}/cmake/version.h.in
        ${PROJECT_VERSION_INCLUDE_DIR}/version.h
        @ONLY
)
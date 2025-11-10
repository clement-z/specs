##################################
# Download and install GoogleTest
include(FetchContent)

FetchContent_Declare(
        fmt
        GIT_REPOSITORY https://github.com/fmtlib/fmt
        GIT_TAG        v11.1.3
        GIT_SHALLOW    YES
        FIND_PACKAGE_ARGS NAMES fmt
        # Other content options...
        EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(fmt)
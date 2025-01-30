##################################
# Download and install GoogleTest
include(FetchContent)

set(INSTALL_GMOCK OFF)
set(INSTALL_GTEST OFF)
FetchContent_Declare(
        googletest
        #URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        15460959cbbfa20e66ef0b5ab497367e47fc0a04 # release-1.12.0
        GIT_SHALLOW YES
        FIND_PACKAGE_ARGS NAMES GTest
        # Other content options...
        EXCLUDE_FROM_ALL
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
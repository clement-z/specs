##################################
# Download and install magic_enum
include(FetchContent)

FetchContent_Declare(
        magic_enum
        URL https://github.com/Neargye/magic_enum/archive/refs/tags/v0.9.6.zip
        # Other content options...
        EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(magic_enum)
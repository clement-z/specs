if(BUILD_TB)
    # Set up GoogleTest (or add_subdirectory if your cmake/googletest.cmake does that)
    include(${CMAKE_SOURCE_DIR}/cmake/googletest.cmake)

    enable_testing()

    add_executable(unit_tests
            main.cpp
    )
    # Compiler options & definitions for the tests
    target_compile_options(unit_tests
            PRIVATE
            -Wall
            -Wextra
            -pedantic
            -Werror
    )
    target_link_libraries(unit_tests
            PRIVATE
            GTest::gtest_main
            common
            ${LIB_NAME}
    )

    #add_coverage_target(TARGET unit_tests EXCLUDE_PATH "*/test/*")

    include(GoogleTest)
    gtest_discover_tests(unit_tests)
endif()
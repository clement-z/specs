function(add_coverage_target)
    set(options)
    set(oneValueArgs TARGET)
    set(multiValueArgs EXCLUDE_PATH BUILD_TYPES)
    cmake_parse_arguments(PARSE_ARGV 0 arg
            "${options}" "${oneValueArgs}" "${multiValueArgs}"
    )

    # The above will set or unset variables with the following names:
    #   arg_TARGET
    #   arg_EXCLUDE_PATH
    #   arg_BUILD_TYPES

    # Exclude some external paths by default
    list(APPEND arg_EXCLUDE_PATH "${CMAKE_CURRENT_BINARY_DIR}/_deps/*" "/usr/include/*")

    # Enable for Debug by default if unspecified
    if (NOT arg_BUILD_TYPES)
        list(APPEND arg_BUILD_TYPES Debug)
    endif()

    # If current build type is not in the specified build types, return
    if(NOT ${CMAKE_BUILD_TYPE} IN_LIST arg_BUILD_TYPES)
        return()
    endif()

    message(STATUS ${CMAKE_CXX_COMPILER_ID})
    if (NOT ${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
        message(WARNING "Code coverage analysis is only supported for GNU toolchains")
        return()
    endif()

    find_program(GCOV gcov)
    if (NOT GCOV)
        message(WARNING "program gcov not found")
    endif()

    find_program(LCOV lcov)
    if (NOT LCOV)
        message(WARNING "program lcov not found")
    endif()

    find_program(GENHTML genhtml)
    if (NOT GENHTML)
        message(WARNING "program genhtml not found")
    endif()

    if (LCOV AND GCOV AND GENHTML)
        set(covname ${arg_TARGET}.cov.info)
        set(covdir coverage/${arg_TARGET})
        set(report  ${arg_TARGET}_cov)

        # Define the custom command to generate coverage
        add_custom_command(
                DEPENDS ${arg_TARGET}
                OUTPUT  ${covname}
                OUTPUT  ${covdir}/index.html
                COMMAND ./${arg_TARGET} > /dev/null
                COMMAND ${LCOV} --ignore-errors inconsistent,inconsistent -c -o ${covname} -d . -b . --gcov-tool ${GCOV} > /dev/null
                COMMAND ${LCOV} --ignore-errors unused -r ${covname} -o ${covname} ${arg_EXCLUDE_PATH} > /dev/null
                COMMAND ${LCOV} -l ${covname} > /dev/null
                COMMAND ${GENHTML} ${covname} --demangle-cpp -output ${covdir} > /dev/null
                COMMAND ${LCOV} --summary "${covname}" | sed -e "1d" > ${covdir}/summary.txt
                DEPENDS ${TARGET}
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                VERBATIM
        )

        # Create a custom target that depends on the coverage info
        add_custom_target(${report}
                DEPENDS ${covname}
                DEPENDS ${covdir}/index.html
                COMMAND echo "Report available at file://${CMAKE_BINARY_DIR}/${covdir}/index.html"
                COMMAND cat "${covdir}/summary.txt"
                COMMENT "Generating coverage report."
        )

        add_dependencies(${report} ${arg_TARGET})

        set_directory_properties(PROPERTIES
                ADDITIONAL_CLEAN_FILES "${covname};${covdir}"
        )

        # Add coverage compile and link options to your target
        target_compile_options(${arg_TARGET} PRIVATE -fprofile-arcs -ftest-coverage)
        target_link_options(${arg_TARGET} PRIVATE --coverage)
    else()
        message(WARNING "unable to add target `${report}`: missing coverage tools")
    endif()

endfunction()
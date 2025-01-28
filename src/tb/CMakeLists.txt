set(SRC_FILES "alltestbenches.cpp")
set(HH_FILES  "alltestbenches.h")


add_library(${PROJECT_NAME}_tb OBJECT EXCLUDE_FROM_ALL
        ${SRC_FILES}
        ${HH_FILES}
)

if(BUILD_TB)
    target_compile_definitions(${PROJECT_NAME}_tb PUBLIC "BUILD_TB=1")

    # Glob all *_tb.cpp files in the current directory
    file(GLOB TB_CPP_FILES
            RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
            CONFIGURE_DEPENDS
            "*_tb.cpp"
    )

    # Glob all *_tb.h files in the current directory
    file(GLOB TB_H_FILES
            RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
            CONFIGURE_DEPENDS
            "*_tb.h"
    )

    # Append the discovered test-bench files to your base lists
    list(APPEND SRC_FILES ${TB_CPP_FILES})
    list(APPEND HH_FILES  ${TB_H_FILES})
endif()

target_sources(${PROJECT_NAME}_tb PRIVATE
        ${SRC_FILES}
        ${HH_FILES}
)
target_link_libraries(${PROJECT_NAME}_tb PRIVATE common)
target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}_tb)
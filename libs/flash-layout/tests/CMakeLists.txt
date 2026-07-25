include(${CMAKE_CURRENT_LIST_DIR}/../../../cmake/midismith_add_test.cmake)

midismith_add_test(NAME flash_layout
    SOURCES
        flash_layout.test.cpp
)

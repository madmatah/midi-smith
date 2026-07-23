include(${CMAKE_CURRENT_LIST_DIR}/../../../cmake/midismith_add_test.cmake)

midismith_add_test(NAME text_display
    SOURCES
        text_display_requirements.test.cpp
)

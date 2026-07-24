include(${CMAKE_CURRENT_LIST_DIR}/../../../cmake/midismith_add_test.cmake)

midismith_add_test(NAME menu
    SOURCES
        menu_stack.test.cpp
        list_screen.test.cpp
        numeric_input_screen.test.cpp
        text_view_screen.test.cpp
        menu_runtime.test.cpp
        line_buffer_stream.test.cpp
        progress_bar.test.cpp
        title_bar.test.cpp
)

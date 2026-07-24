cmake_minimum_required(VERSION 3.22)
project(menu LANGUAGES CXX)

include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/midismith_add_lib.cmake)

midismith_add_lib(NAME menu
    SOURCES
        src/menu_stack.cpp
        src/menu_runtime.cpp
        src/list_screen.cpp
        src/numeric_input_screen.cpp
        src/text_view_screen.cpp
        src/progress_bar.cpp
        src/line_buffer.cpp
        src/line_buffer_stream.cpp
    DEPENDS text-display io
)

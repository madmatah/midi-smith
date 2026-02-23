cmake_minimum_required(VERSION 3.22)
project(shell LANGUAGES CXX)

include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/midismith_add_lib.cmake)

midismith_add_lib(NAME shell
    SOURCES
        src/command_parser.cpp
        src/command_dispatcher.cpp
        src/line_editor.cpp
        src/shell_engine.cpp
    DEPENDS io
)

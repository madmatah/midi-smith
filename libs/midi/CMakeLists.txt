cmake_minimum_required(VERSION 3.22)
project(midi LANGUAGES CXX)

include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/midismith_add_lib.cmake)

midismith_add_lib(NAME midi
    SOURCES
        src/midi_fanout_controller.cpp
        src/midi_input_parser.cpp
        src/note_name.cpp
)

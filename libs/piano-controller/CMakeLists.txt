cmake_minimum_required(VERSION 3.22)
project(piano_controller LANGUAGES CXX)

include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/midismith_add_lib.cmake)

midismith_add_lib(NAME piano_controller
    SOURCES src/midi_piano.cpp
    DEPENDS midi
)

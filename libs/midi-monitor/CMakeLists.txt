cmake_minimum_required(VERSION 3.22)
project(midi_monitor LANGUAGES CXX)

include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/midismith_add_lib.cmake)

midismith_add_lib(NAME midi_monitor
    SOURCES src/midi_activity_collector.cpp
    DEPENDS midi
)

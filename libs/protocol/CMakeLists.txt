cmake_minimum_required(VERSION 3.22)
project(protocol LANGUAGES CXX)

include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/midismith_add_lib.cmake)

midismith_add_lib(NAME protocol
    SOURCES
        src/messages.cpp
        src/message_parser.cpp
        src/peer_monitor.cpp
    DEPENDS
        byte-codec
)

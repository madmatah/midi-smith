cmake_minimum_required(VERSION 3.22)
project(protocol-can LANGUAGES CXX)

include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/midismith_add_lib.cmake)

midismith_add_lib(NAME protocol_can
    DEPENDS
        can-broker
        protocol
        stats
)

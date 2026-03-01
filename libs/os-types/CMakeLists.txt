cmake_minimum_required(VERSION 3.22)
project(os_types LANGUAGES CXX)

include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/midismith_add_lib.cmake)

midismith_add_lib(NAME os_types
    DEPENDS
        stats
)

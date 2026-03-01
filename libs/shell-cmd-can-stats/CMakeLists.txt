cmake_minimum_required(VERSION 3.22)
project(shell_cmd_can_stats LANGUAGES CXX)

include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/midismith_add_lib.cmake)

midismith_add_lib(NAME shell_cmd_can_stats
    SOURCES
        src/can_stats_command.cpp
    DEPENDS
        shell
        bsp-types
)

if(UNIT_TESTS)
    add_subdirectory(tests)
endif()

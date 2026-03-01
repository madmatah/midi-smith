cmake_minimum_required(VERSION 3.22)
project(shell_cmd_os_stats LANGUAGES CXX)

include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/midismith_add_lib.cmake)

midismith_add_lib(NAME shell_cmd_os_stats
    SOURCES
        src/ps_command.cpp
        src/status_command.cpp
    DEPENDS
        shell-cmd-stats
        os-types
)

if(UNIT_TESTS)
    add_subdirectory(tests)
endif()

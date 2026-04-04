cmake_minimum_required(VERSION 3.22)
project(shell_cmd_reboot LANGUAGES CXX)

include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/midismith_add_lib.cmake)

midismith_add_lib(NAME shell_cmd_reboot
    SOURCES
        src/reboot_command.cpp
    DEPENDS
        shell
        bsp-types
)

if(UNIT_TESTS)
    add_subdirectory(tests)
endif()

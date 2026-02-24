cmake_minimum_required(VERSION 3.22)
project(shell_cmd_version LANGUAGES CXX)

include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/midismith_add_lib.cmake)

midismith_add_lib(NAME shell_cmd_version
    SOURCES
        src/version_command.cpp
    DEPENDS shell
)

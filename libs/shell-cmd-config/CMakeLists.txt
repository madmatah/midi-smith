cmake_minimum_required(VERSION 3.22)
project(shell_cmd_config LANGUAGES CXX)

include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/midismith_add_lib.cmake)

midismith_add_lib(NAME shell_cmd_config
    SOURCES src/config_command.cpp
    DEPENDS shell config
)

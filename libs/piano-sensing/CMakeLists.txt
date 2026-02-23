cmake_minimum_required(VERSION 3.22)
project(piano_sensing LANGUAGES CXX)

include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/midismith_add_lib.cmake)

midismith_add_lib(NAME piano_sensing DEPENDS midi)

cmake_minimum_required(VERSION 3.22)
project(update_catalogue LANGUAGES CXX)

include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/midismith_add_lib.cmake)

midismith_add_lib(NAME update_catalogue
    SOURCES
        src/update_catalogue.cpp
    DEPENDS firmware-image product-id
)

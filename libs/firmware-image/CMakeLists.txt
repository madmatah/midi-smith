cmake_minimum_required(VERSION 3.22)
project(firmware_image LANGUAGES CXX)

include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/midismith_add_lib.cmake)

midismith_add_lib(NAME firmware_image
    SOURCES
        src/image_acceptance.cpp
        src/image_header.cpp
    DEPENDS byte-codec
)

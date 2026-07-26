cmake_minimum_required(VERSION 3.22)
project(firmware_installer LANGUAGES CXX)

include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/midismith_add_lib.cmake)

midismith_add_lib(NAME firmware_installer
    SOURCES
        src/staged_image_installer.cpp
    DEPENDS checksum firmware-image product-id
)

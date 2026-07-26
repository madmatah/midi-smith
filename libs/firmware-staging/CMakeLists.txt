cmake_minimum_required(VERSION 3.22)
project(firmware_staging LANGUAGES CXX)

include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/midismith_add_lib.cmake)

midismith_add_lib(NAME firmware_staging
    SOURCES
        src/staging_writer.cpp
    DEPENDS firmware-image product-id
)

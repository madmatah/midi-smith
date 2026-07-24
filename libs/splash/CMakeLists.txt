cmake_minimum_required(VERSION 3.22)
project(splash LANGUAGES CXX)

include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/midismith_add_lib.cmake)

midismith_add_lib(NAME splash
    SOURCES
        src/pixel_canvas.cpp
        src/animation.cpp
)

if(CMAKE_CROSSCOMPILING)
    target_compile_options(splash PRIVATE -O2)
endif()

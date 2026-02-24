include_guard(GLOBAL)

cmake_minimum_required(VERSION 3.22)

option(HOST_TESTS "Build host-only unit tests" OFF)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

enable_language(C ASM CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

if(NOT HOST_TESTS)
    list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)
endif()

function(midismith_firmware_link_map TARGET)
    target_link_options(${TARGET} PRIVATE -Wl,-Map=${TARGET}.map)
endfunction()

macro(midismith_add_stm32cubemx TARGET)
    # CubeMX uses ${CMAKE_PROJECT_NAME} to refer to the firmware target, but when built from the
    # monorepo root, CMAKE_PROJECT_NAME resolves to the root project name ("midi-smith-monorepo").
    # Shadowing it here ensures the CubeMX-generated CMakeLists.txt targets the correct executable,
    # regardless of whether the package is built standalone or from the monorepo root.
    set(CMAKE_PROJECT_NAME "${TARGET}")
    add_subdirectory(cmake/stm32cubemx)
endmacro()

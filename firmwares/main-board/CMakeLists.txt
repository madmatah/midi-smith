cmake_minimum_required(VERSION 3.22)

option(HOST_TESTS "Build host-only unit tests" OFF)

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

set(MAIN_FIRMWARE_TARGET "main-board")
set(MAIN_TARGET_PREFIX "main")
set(MAIN_PROJECT_ROOT "${CMAKE_CURRENT_LIST_DIR}")

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${MAIN_FIRMWARE_TARGET})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM CXX)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

set(MAIN_COMMON_INCLUDE_DIRS "${MAIN_PROJECT_ROOT}/../../libs/common/include")
set(MAIN_COMMON_SRC_DIRS "${MAIN_PROJECT_ROOT}/../../libs/common/src")

set(MAIN_SEGGER_ROOT "${MAIN_PROJECT_ROOT}/../../third_party/SEGGER")

if(HOST_TESTS)
    enable_testing()

    add_library(main_domain STATIC)
    target_sources(main_domain PRIVATE
        domain/src/music/piano/midi_piano.cpp
    )
    target_include_directories(main_domain PUBLIC
        domain/include
        ${MAIN_COMMON_INCLUDE_DIRS}
    )

    add_library(main_app_lib STATIC)
    target_sources(main_app_lib PRIVATE
        app/src/app/midi/async_task_midi_controller.cpp
    )
    target_include_directories(main_app_lib PUBLIC
        app/include
        os/include
        ${MAIN_COMMON_INCLUDE_DIRS}
    )
    target_link_libraries(main_app_lib PRIVATE main_domain)

    if(PROJECT_IS_TOP_LEVEL)
        add_library(domain ALIAS main_domain)
        add_library(app_lib ALIAS main_app_lib)
    endif()

    add_subdirectory(tests)
else()
# Create an executable object type
add_executable(${MAIN_FIRMWARE_TARGET})

# Add compile options for embedded C++
target_compile_options(${MAIN_FIRMWARE_TARGET} PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions -fno-rtti>
)

# Add STM32CubeMX generated sources
# CubeMX uses ${CMAKE_PROJECT_NAME} to refer to the firmware target, but when built from the
# monorepo root, CMAKE_PROJECT_NAME resolves to the root project name ("midi-smith-monorepo").
# Shadowing it here ensures the CubeMX-generated CMakeLists.txt targets the correct executable,
# regardless of whether the package is built standalone or from the monorepo root.
set(CMAKE_PROJECT_NAME "${MAIN_FIRMWARE_TARGET}")
add_subdirectory(cmake/stm32cubemx)

# Link directories setup
target_link_directories(${MAIN_FIRMWARE_TARGET} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${MAIN_FIRMWARE_TARGET} PRIVATE
    # Add user sources here
    app/src/application.cpp
    app/src/app_entry.cpp
    app/src/tasks/led_task.cpp
    bsp/src/board.cpp
    ${MAIN_COMMON_SRC_DIRS}/bsp/gpio.cpp
    ${MAIN_COMMON_SRC_DIRS}/bsp/rtt_logger.cpp
    bsp/src/rtt_telemetry_sender.cpp
    bsp/src/spi.cpp
    bsp/src/stm32_octospi.cpp
    bsp/src/stm32_spi_flash.cpp
    bsp/src/usb_midi.cpp
    bsp/src/usb_midi_init.cpp
    bsp/src/usb_midi/usbd_midi.c
    app/src/app/midi/async_task_midi_controller.cpp
    app/src/app/midi/midi_task.cpp
    domain/src/music/piano/midi_piano.cpp
    ${MAIN_SEGGER_ROOT}/RTT/RTT/SEGGER_RTT.c
    ${MAIN_SEGGER_ROOT}/RTT/RTT/SEGGER_RTT_printf.c
    ${MAIN_COMMON_SRC_DIRS}/os/clock.cpp
    ${MAIN_COMMON_SRC_DIRS}/os/task.cpp
    ${MAIN_COMMON_SRC_DIRS}/os/queue.cpp
    ${MAIN_COMMON_SRC_DIRS}/os/binary_semaphore.cpp
    ${MAIN_COMMON_SRC_DIRS}/os/memory.cpp
    ${MAIN_COMMON_SRC_DIRS}/os/this_task.cpp
)

# Add include paths
target_include_directories(${MAIN_FIRMWARE_TARGET} PRIVATE
    # Add user defined include paths
    app/include
    bsp/include
    bsp/include/bsp/usb_midi
    ${MAIN_SEGGER_ROOT}/RTT/RTT
    ${MAIN_SEGGER_ROOT}/RTT/Config
    os/include
    domain/include
    ${MAIN_COMMON_INCLUDE_DIRS}
)

target_compile_definitions(${MAIN_FIRMWARE_TARGET} PRIVATE
    SEGGER_RTT_SECTION=\".rtt\"
)

# Add project symbols (macros)
target_compile_definitions(${MAIN_FIRMWARE_TARGET} PRIVATE
    # Add user defined symbols
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_options(${MAIN_FIRMWARE_TARGET} PRIVATE
    -Wl,-Map=${MAIN_FIRMWARE_TARGET}.map
)

target_link_libraries(${MAIN_FIRMWARE_TARGET}
    stm32cubemx

    # Add user defined libraries
)
endif()

# Update build/current symlink after each preset change
if(PROJECT_IS_TOP_LEVEL AND CMAKE_EXPORT_COMPILE_COMMANDS)
    file(MAKE_DIRECTORY "${CMAKE_SOURCE_DIR}/build")
    message(STATUS "Updating build/current symlink to : ${CMAKE_BINARY_DIR}")
    execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink
        "${CMAKE_BINARY_DIR}"
        "${CMAKE_SOURCE_DIR}/build/current")
endif()

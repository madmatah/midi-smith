cmake_minimum_required(VERSION 3.22)
include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/midismith_firmware_common.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/midismith_require_lib.cmake)

set(MAIN_FIRMWARE_TARGET "main-board")
set(MAIN_TARGET_PREFIX "main")
set(MAIN_PROJECT_ROOT "${CMAKE_CURRENT_LIST_DIR}")

project(${MAIN_FIRMWARE_TARGET})
message("Build type: " ${CMAKE_BUILD_TYPE})

midismith_require_lib(midi)
midismith_require_lib(piano-controller)
midismith_require_lib(logging)
midismith_require_lib(bsp)
midismith_require_lib(os)
midismith_require_lib(telemetry)

if(HOST_TESTS)
    enable_testing()

    add_library(main_domain INTERFACE)
    target_include_directories(main_domain INTERFACE
        domain/include
    )
    target_link_libraries(main_domain INTERFACE piano_controller midi bsp os telemetry)

    add_library(main_app_lib STATIC)
    target_sources(main_app_lib PRIVATE
        app/src/app/midi/async_task_midi_controller.cpp
    )
    target_include_directories(main_app_lib PUBLIC
        app/include
    )
    target_link_libraries(main_app_lib PRIVATE main_domain piano_controller midi logging)

    if(PROJECT_IS_TOP_LEVEL)
        add_library(domain ALIAS main_domain)
        add_library(app_lib ALIAS main_app_lib)
    endif()

    add_subdirectory(tests)
else()
# Create an executable object type
add_executable(${MAIN_FIRMWARE_TARGET})

include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/midismith_firmware_versioning.cmake)
midismith_firmware_versioning(${MAIN_FIRMWARE_TARGET})
midismith_add_stm32cubemx(${MAIN_FIRMWARE_TARGET})

# Add sources to executable
target_sources(${MAIN_FIRMWARE_TARGET} PRIVATE
    # Add user sources here
    app/src/application.cpp
    app/src/app_entry.cpp
    app/src/version.cpp
    app/src/tasks/led_task.cpp
    bsp/src/board.cpp
    bsp/src/spi.cpp
    bsp/src/stm32_octospi.cpp
    bsp/src/stm32_spi_flash.cpp
    bsp/src/usb_midi.cpp
    bsp/src/usb_midi_init.cpp
    bsp/src/usb_midi/usbd_midi.c
    app/src/app/midi/async_task_midi_controller.cpp
    app/src/app/midi/midi_task.cpp
)

# Add include paths
target_include_directories(${MAIN_FIRMWARE_TARGET} PRIVATE
    # Add user defined include paths
    app/include
    bsp/include
    bsp/include/bsp/usb_midi
    domain/include
)

midismith_firmware_link_map(${MAIN_FIRMWARE_TARGET})

target_link_libraries(${MAIN_FIRMWARE_TARGET}
    stm32cubemx
    midi
    piano_controller
    bsp
    logging
    os
    telemetry

    # Add user defined libraries
)
endif()

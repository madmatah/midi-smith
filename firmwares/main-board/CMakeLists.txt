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

if(HOST_TESTS)
    enable_testing()

    add_library(main_domain INTERFACE)
    target_include_directories(main_domain INTERFACE
        domain/include
    )
    target_link_libraries(main_domain INTERFACE piano_controller midi bsp os)

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

# Add sources to executable
target_sources(${MAIN_FIRMWARE_TARGET} PRIVATE
    # Add user sources here
    app/src/application.cpp
    app/src/app_entry.cpp
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

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_options(${MAIN_FIRMWARE_TARGET} PRIVATE
    -Wl,-Map=${MAIN_FIRMWARE_TARGET}.map
)

target_link_libraries(${MAIN_FIRMWARE_TARGET}
    stm32cubemx
    midi
    piano_controller
    bsp
    logging
    os

    # Add user defined libraries
)
endif()

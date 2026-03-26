cmake_minimum_required(VERSION 3.22)
include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/midismith_firmware_common.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/midismith_require_lib.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/midismith_coverage.cmake)

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
midismith_require_lib(io)
midismith_require_lib(shell)
midismith_require_lib(shell-cmd-version)
midismith_require_lib(shell-cmd-os-stats)
midismith_require_lib(shell-cmd-stats)
midismith_require_lib(stats)
midismith_require_lib(os-types)
midismith_require_lib(bsp-types)
midismith_require_lib(can-broker)
midismith_require_lib(protocol)
midismith_require_lib(protocol-can)
midismith_require_lib(config)
midismith_require_lib(sensor-linearization)

if(HOST_TESTS)
    enable_testing()

    add_library(main_domain STATIC
        domain/src/main_board_config.cpp
        domain/src/keymap/keymap_lookup.cpp
        domain/src/keymap/keymap_setup_session.cpp
        domain/src/calibration/calibration_session.cpp
    )
    target_include_directories(main_domain PUBLIC domain/include)
    target_link_libraries(main_domain PUBLIC config piano_controller midi bsp os telemetry
                                             sensor_linearization)

    add_library(main_app_lib STATIC)
    target_sources(main_app_lib PRIVATE
        app/src/app/midi/async_task_midi_controller.cpp
        app/src/messaging/main_board_can_message_sender.cpp
        app/src/messaging/main_board_inbound_sensor_event_logging_handler.cpp
        app/src/messaging/main_board_inbound_sensor_event_midi_handler.cpp
        app/src/shell/can_command.cpp
        app/src/shell/adc_command.cpp
        app/src/shell/keymap_command.cpp
        app/src/shell/calibration_command.cpp
        app/src/supervisor/network_supervisor_task.cpp
        app/src/adc/adc_board_controller.cpp
        app/src/storage/main_board_persistent_configuration.cpp
        app/src/keymap/keymap_setup_coordinator.cpp
        app/src/messaging/main_board_inbound_keymap_capture_handler.cpp
        app/src/calibration/calibration_bulk_data_receiver.cpp
        app/src/calibration/calibration_coordinator.cpp
        app/src/storage/calibration_persistent_store.cpp
    )
    target_include_directories(main_app_lib PUBLIC
        app/include
    )
    target_link_libraries(main_app_lib PUBLIC protocol protocol_can shell io bsp_types shell_cmd_stats stats)
    target_link_libraries(main_app_lib PRIVATE main_domain piano_controller midi logging)
    midismith_enable_coverage_compile(main_app_lib)

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
    app/src/tasks/shell_task.cpp
    app/src/tasks/config_storage_task.cpp
    app/src/composition/can_subsystem.cpp
    app/src/composition/midi_subsystem.cpp
    app/src/composition/shell_subsystem.cpp
    app/src/composition/config_subsystem.cpp
    app/src/supervisor/network_supervisor_task.cpp
    app/src/composition/supervisor_subsystem.cpp
    bsp/src/board.cpp
    bsp/src/spi.cpp
    bsp/src/stm32_octospi.cpp
    bsp/src/stm32_spi_flash.cpp
    bsp/src/storage/spi_flash_sector_storage.cpp
    bsp/src/usb_midi.cpp
    bsp/src/usb_midi_init.cpp
    bsp/src/usb_midi/usbd_midi.c
    bsp/src/serial/uart_stream_registry.cpp
    bsp/src/cortex/axi_sram_nocache_mpu.cpp
    app/src/app/midi/async_task_midi_controller.cpp
    app/src/app/midi/midi_task.cpp
    app/src/messaging/main_board_can_message_sender.cpp
    app/src/messaging/main_board_inbound_sensor_event_logging_handler.cpp
    app/src/messaging/main_board_inbound_sensor_event_midi_handler.cpp
    app/src/shell/can_command.cpp
    app/src/shell/adc_command.cpp
    app/src/shell/keymap_command.cpp
    app/src/shell/calibration_command.cpp
    app/src/adc/adc_board_controller.cpp
    app/src/storage/main_board_persistent_configuration.cpp
    app/src/keymap/keymap_setup_coordinator.cpp
    app/src/messaging/main_board_inbound_keymap_capture_handler.cpp
    app/src/calibration/calibration_bulk_data_receiver.cpp
    app/src/calibration/calibration_coordinator.cpp
    app/src/storage/calibration_persistent_store.cpp
    app/src/composition/calibration_subsystem.cpp
    domain/src/main_board_config.cpp
    domain/src/keymap/keymap_lookup.cpp
    domain/src/keymap/keymap_setup_session.cpp
    domain/src/calibration/calibration_session.cpp
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
    io
    shell
    shell_cmd_version
    shell_cmd_os_stats
    shell_cmd_stats
    stats
    can_broker
    protocol
    protocol_can
    config
    sensor_linearization

    # Add user defined libraries
)
endif()

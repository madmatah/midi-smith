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
midismith_require_lib(midi-monitor)
midismith_require_lib(piano-controller)
midismith_require_lib(logging)
midismith_require_lib(bsp)
midismith_require_lib(os)
midismith_require_lib(telemetry)
midismith_require_lib(io)
midismith_require_lib(text-display)
midismith_require_lib(menu)
midismith_require_lib(splash)
midismith_require_lib(shell)
midismith_require_lib(shell-cmd-version)
midismith_require_lib(shell-cmd-os-stats)
midismith_require_lib(shell-cmd-reboot)
midismith_require_lib(shell-cmd-stats)
midismith_require_lib(stats)
midismith_require_lib(os-types)
midismith_require_lib(bsp-types)
midismith_require_lib(flash-layout)
midismith_require_lib(can-broker)
midismith_require_lib(protocol)
midismith_require_lib(protocol-can)
midismith_require_lib(config)
midismith_require_lib(sensor-linearization)
midismith_require_lib(calibration)
midismith_require_lib(update-catalogue)
midismith_require_lib(firmware-staging)
midismith_require_lib(bsp-flash)
midismith_require_lib(boot-control)

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
                                             sensor_linearization calibration)

    add_library(main_app_lib STATIC)
    target_sources(main_app_lib PRIVATE
        app/src/app/midi/async_task_midi_controller.cpp
        app/src/app/midi/midi_input_task.cpp
        app/src/messaging/main_board_can_message_sender.cpp
        app/src/messaging/main_board_inbound_sensor_event_logging_handler.cpp
        app/src/messaging/main_board_inbound_sensor_event_midi_handler.cpp
        app/src/shell/can_command.cpp
        app/src/shell/adc_command.cpp
        app/src/shell/keymap_command.cpp
        app/src/shell/sdcard_command.cpp
        app/src/update/self_update_service.cpp
        app/src/shell/calibration_command.cpp
        app/src/supervisor/network_supervisor_task.cpp
        app/src/adc/adc_board_controller.cpp
        app/src/storage/main_board_persistent_configuration.cpp
        app/src/keymap/keymap_setup_coordinator.cpp
        app/src/messaging/main_board_inbound_keymap_capture_handler.cpp
        app/src/calibration/calibration_bulk_data_receiver.cpp
        app/src/calibration/calibration_data_server_task.cpp
        app/src/calibration/calibration_coordinator.cpp
        app/src/storage/calibration_persistent_store.cpp
        app/src/app/ui/screens/calibration_progress_screen.cpp
        app/src/app/ui/screens/midi_monitor_screen.cpp
        app/src/app/ui/midi_activity_wake_source.cpp
        app/src/app/ui/font_8x16.cpp
        app/src/app/ui/tft_text_display.cpp
        app/src/app/ui/ui_task.cpp
        app/src/app/ui/items/calibration_flow_item.cpp
        app/src/app/ui/items/keymap_setup_flow_item.cpp
        app/src/app/ui/items/stats_view_item.cpp
        app/src/app/ui/screens/keymap_progress_screen.cpp
        app/src/app/ui/screens/persistent_config_view.cpp
        app/src/app/ui/tft_splash_player.cpp
        app/src/shell/ui_command.cpp
    )
    target_include_directories(main_app_lib PUBLIC
        app/include
    )
    target_link_libraries(main_app_lib PUBLIC protocol protocol_can shell io bsp_types shell_cmd_stats stats calibration shell_cmd_reboot menu text_display splash midi_monitor update_catalogue firmware_staging boot_control flash_layout)
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

include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/midismith_flash_layout.cmake)
midismith_firmware_load_address(${MAIN_FIRMWARE_TARGET})

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
    bsp/src/stm32_spi_flash.cpp
    bsp/src/storage/spi_flash_sector_storage.cpp
    bsp/src/storage/sd_card_bring_up.cpp
    bsp/src/storage/staging_slot_flash.cpp
    bsp/src/storage/sd_card_image_source.cpp
    bsp/src/usb_midi.cpp
    bsp/src/usb_midi_init.cpp
    bsp/src/usart_midi.cpp
    bsp/src/tft_display.cpp
    bsp/src/rotary_encoder.cpp
    bsp/src/rotary_button.cpp
    bsp/src/usb_midi/usbd_midi.c
    bsp/src/serial/uart_stream_registry.cpp
    bsp/src/cortex/axi_sram_nocache_mpu.cpp
    bsp/src/cortex/d3_sram_nocache_mpu.cpp
    app/src/app/midi/async_task_midi_controller.cpp
    app/src/app/midi/midi_output_task.cpp
    app/src/app/midi/midi_input_task.cpp
    app/src/app/ui/font_8x16.cpp
    app/src/app/ui/tft_splash_player.cpp
    app/src/app/ui/tft_text_display.cpp
    app/src/app/ui/ui_task.cpp
    app/src/composition/menu_tree.cpp
    app/src/app/ui/items/keymap_setup_flow_item.cpp
    app/src/app/ui/items/calibration_flow_item.cpp
    app/src/app/ui/items/stats_view_item.cpp
    app/src/app/ui/screens/keymap_progress_screen.cpp
    app/src/app/ui/screens/calibration_progress_screen.cpp
    app/src/app/ui/screens/midi_monitor_screen.cpp
    app/src/app/ui/midi_activity_wake_source.cpp
    app/src/app/ui/screens/persistent_config_view.cpp
    app/src/messaging/main_board_can_message_sender.cpp
    app/src/messaging/main_board_inbound_sensor_event_logging_handler.cpp
    app/src/messaging/main_board_inbound_sensor_event_midi_handler.cpp
    app/src/shell/can_command.cpp
    app/src/shell/adc_command.cpp
    app/src/shell/keymap_command.cpp
    app/src/shell/sdcard_command.cpp
    app/src/update/self_update_service.cpp
    app/src/shell/calibration_command.cpp
    app/src/shell/ui_command.cpp
    app/src/shell/os_clock_blocking_delay.cpp
    app/src/adc/adc_board_controller.cpp
    app/src/storage/main_board_persistent_configuration.cpp
    app/src/keymap/keymap_setup_coordinator.cpp
    app/src/messaging/main_board_inbound_keymap_capture_handler.cpp
    app/src/calibration/calibration_bulk_data_receiver.cpp
    app/src/calibration/calibration_data_server_task.cpp
    app/src/calibration/calibration_coordinator.cpp
    app/src/storage/calibration_persistent_store.cpp
    app/src/composition/calibration_subsystem.cpp
    app/src/composition/ui_subsystem.cpp
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
    midi_monitor
    piano_controller
    bsp
    logging
    os
    telemetry
    io
    text_display
    menu
    splash
    shell
    shell_cmd_version
    shell_cmd_os_stats
    shell_cmd_reboot
    shell_cmd_stats
    stats
    can_broker
    protocol
    protocol_can
    config
    sensor_linearization
    calibration
    flash_layout
    update_catalogue
    firmware_staging
    bsp_flash
    boot_control

    # Add user defined libraries
)


midismith_check_flash_layout(${MAIN_FIRMWARE_TARGET})

include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/midismith_firmware_package.cmake)
midismith_firmware_package(${MAIN_FIRMWARE_TARGET} PRODUCT main-board)
endif()

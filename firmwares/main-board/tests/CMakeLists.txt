include(${CMAKE_CURRENT_LIST_DIR}/../../../cmake/midismith_add_firmware_test.cmake)

midismith_add_firmware_test(NAME main
    SOURCES
        domain/config/main_board_config.test.cpp
        app/midi/async_task_midi_controller.test.cpp
        app/messaging/main_board_can_message_sender.test.cpp
        app/messaging/main_board_inbound_sensor_event_logging_handler.test.cpp
        app/messaging/main_board_inbound_sensor_event_midi_handler.test.cpp
        app/messaging/main_board_inbound_keymap_capture_handler.test.cpp
        app/shell/can_command.test.cpp
        app/shell/adc_command.test.cpp
        app/shell/keymap_command.test.cpp
        app/supervisor/network_supervisor_task.test.cpp
        app/adc/adc_board_state.test.cpp
        app/adc/adc_board_controller.test.cpp
        app/adc/adc_boards_controller.test.cpp
        app/storage/main_board_persistent_configuration.test.cpp
        app/keymap/keymap_setup_coordinator.test.cpp
        domain/keymap/keymap_lookup.test.cpp
        domain/keymap/keymap_setup_session.test.cpp
        domain/calibration/calibration_session.test.cpp
    LINK_LIBRARIES main_domain main_app_lib
    USE_FAKEIT
)

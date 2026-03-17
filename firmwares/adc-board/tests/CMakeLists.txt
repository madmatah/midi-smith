include(${CMAKE_CURRENT_LIST_DIR}/../../../cmake/midismith_add_firmware_test.cmake)

midismith_add_firmware_test(NAME adc
    SOURCES
        domain/sensors/processed_sensor_group.test.cpp
        domain/sensors/sensor_registry.test.cpp
        domain/sensors/capture_sensor_state.test.cpp
        domain/sensors/sensor_member_reader.test.cpp
        domain/sensors/sensor_state.test.cpp
        domain/sensors/calibration_state_utils.test.cpp
        domain/calibration/calibration_data_collector.test.cpp
        domain/config/adc_board_config.test.cpp
        app/storage/adc_board_persistent_configuration.test.cpp
        app/analog/adc_rank_mapped_frame_decoder.test.cpp
        app/analog/acquisition_sequencer.test.cpp
        app/telemetry/sensor_rtt_stream_capture.test.cpp
        app/telemetry/sensor_rtt_schema_frame_builder.test.cpp
        app/shell/commands/sensor_rtt_command.test.cpp
        app/can/can_filter_updater.test.cpp
        app/messaging/adc_board_can_message_sender.test.cpp
        app/messaging/adc_inbound_command_handler.test.cpp
        app/supervisor/adc_supervisor_task.test.cpp
        app/piano_sensing/logging_sensor_event_handler.test.cpp
        app/piano_sensing/remote_message_sensor_event_handler.test.cpp
        app/analog/signal_processing/calibration_workflow_stages.test.cpp
        app/calibration/calibration_task_test.cpp
    LINK_LIBRARIES adc_domain adc_app_lib
    USE_FAKEIT
)

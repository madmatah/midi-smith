include(FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.5.4
)
FetchContent_MakeAvailable(Catch2)

FetchContent_Declare(
    fakeit
    GIT_REPOSITORY https://github.com/eranpeer/FakeIt.git
    GIT_TAG 2.5.0
)
FetchContent_GetProperties(fakeit)
if(NOT fakeit_POPULATED)
    FetchContent_Populate(fakeit)
endif()

add_library(fakeit INTERFACE)
target_include_directories(fakeit INTERFACE
    ${fakeit_SOURCE_DIR}/single_header/standalone
)

add_executable(unit_tests
    domain/dsp/filters/biquad.test.cpp
    domain/dsp/filters/sg5_smoother.test.cpp
    domain/dsp/filters/ema_filter.test.cpp
    domain/dsp/filters/constant_filter.test.cpp
    domain/dsp/engine/workflow.test.cpp
    domain/dsp/engine/decimated.test.cpp
    domain/dsp/engine/observers.test.cpp
    domain/dsp/engine/temporal_continuity_guard.test.cpp
    domain/dsp/filters/identity_filter.test.cpp
    domain/dsp/logic/input_gate.test.cpp
    domain/dsp/logic/gate_open.test.cpp
    domain/dsp/logic/is_true.test.cpp
    domain/dsp/logic/is_false.test.cpp
    domain/dsp/logic/and.test.cpp
    domain/dsp/logic/or.test.cpp
    domain/dsp/logic/switch.test.cpp
    domain/dsp/converters/tia_current_converter.test.cpp
    domain/dsp/converters/linear_scaler.test.cpp
    domain/dsp/math/sliding_linear_regression.test.cpp
    domain/dsp/math/backward_difference.test.cpp
    domain/dsp/math/central_difference.test.cpp
    domain/shell/command_parser.test.cpp
    domain/shell/line_editor.test.cpp
    domain/shell/command_dispatcher.test.cpp
    domain/shell/shell_engine.test.cpp
    domain/shell/commands/version_command.test.cpp
    domain/sensors/processed_sensor_group.test.cpp
    domain/sensors/sensor_registry.test.cpp
    domain/sensors/linearization/sensor_response_curve.test.cpp
    domain/sensors/linearization/sensor_linear_processor.test.cpp
    domain/sensors/capture_sensor_state.test.cpp
    domain/sensors/sensor_member_reader.test.cpp
    domain/sensors/sensor_state.test.cpp
    domain/music/piano/goebl_logarithmic_velocity_mapper.test.cpp
    domain/music/piano/midi_velocity_engine.test.cpp
    domain/music/piano/note_release_detector_stage.test.cpp
    app/analog/adc_rank_mapped_frame_decoder.test.cpp
    app/analog/acquisition_sequencer.test.cpp
    app/telemetry/sensor_rtt_stream_capture.test.cpp
    app/telemetry/sensor_rtt_schema_frame_builder.test.cpp
    app/shell/commands/sensor_rtt_command.test.cpp
    app/shell/commands/status_command.test.cpp
    app/shell/commands/ps_command.test.cpp
    ${CMAKE_SOURCE_DIR}/app/src/shell/commands/sensor_rtt_command.cpp
    ${CMAKE_SOURCE_DIR}/app/src/shell/commands/status_command.cpp
    ${CMAKE_SOURCE_DIR}/app/src/shell/commands/ps_command.cpp
    ${CMAKE_SOURCE_DIR}/app/src/telemetry/sensor_rtt_schema_frame_builder.cpp
)
target_link_libraries(unit_tests PRIVATE
    Catch2::Catch2WithMain
    fakeit
    domain
)
target_include_directories(unit_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/app/include
    ${CMAKE_SOURCE_DIR}/bsp/include
    ${CMAKE_SOURCE_DIR}/os/include
)
target_compile_definitions(unit_tests PRIVATE UNIT_TESTS=1)
target_compile_features(unit_tests PRIVATE cxx_std_17)

include(Catch)
catch_discover_tests(unit_tests)

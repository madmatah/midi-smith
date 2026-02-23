include(FetchContent)

if(NOT DEFINED ADC_PROJECT_ROOT)
    get_filename_component(ADC_PROJECT_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
endif()

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.5.4
)
FetchContent_MakeAvailable(Catch2)

FetchContent_Declare(
    adc_fakeit_dep
    GIT_REPOSITORY https://github.com/eranpeer/FakeIt.git
    GIT_TAG 2.5.0
)
FetchContent_GetProperties(adc_fakeit_dep)
if(NOT adc_fakeit_dep_POPULATED)
    FetchContent_Populate(adc_fakeit_dep)
endif()

add_library(adc_fakeit INTERFACE)
target_include_directories(adc_fakeit INTERFACE
    ${adc_fakeit_dep_SOURCE_DIR}/single_header/standalone
)

add_executable(adc_unit_tests
    domain/shell/commands/version_command.test.cpp
    domain/sensors/processed_sensor_group.test.cpp
    domain/sensors/sensor_registry.test.cpp
    domain/sensors/capture_sensor_state.test.cpp
    domain/sensors/sensor_member_reader.test.cpp
    domain/sensors/sensor_state.test.cpp
    domain/music/piano/velocity/constant_velocity_mapper.test.cpp
    domain/music/piano/velocity/exponential_velocity_mapper.test.cpp
    domain/music/piano/velocity/goebl_logarithmic_velocity_mapper.test.cpp
    domain/music/piano/velocity/linear_velocity_mapper.test.cpp
    domain/music/piano/velocity/logarithmic_velocity_mapper.test.cpp
    domain/music/piano/midi_velocity_engine.test.cpp
    domain/music/piano/note_release_detector_stage.test.cpp
    domain/hash/crc32.test.cpp
    domain/config/config_validator.test.cpp
    domain/config/adc_board_config.test.cpp
    domain/config/transactional_config_dictionary.test.cpp
    app/storage/storage_manager.test.cpp
    app/storage/adc_board_persistent_configuration.test.cpp
    app/analog/adc_rank_mapped_frame_decoder.test.cpp
    app/analog/acquisition_sequencer.test.cpp
    app/telemetry/sensor_rtt_stream_capture.test.cpp
    app/telemetry/sensor_rtt_schema_frame_builder.test.cpp
    domain/shell/commands/config_command.test.cpp
    app/shell/commands/sensor_rtt_command.test.cpp
    app/shell/commands/status_command.test.cpp
    app/shell/commands/ps_command.test.cpp
    ${ADC_PROJECT_ROOT}/app/src/storage/adc_board_persistent_configuration.cpp
    ${ADC_PROJECT_ROOT}/domain/src/shell/commands/config_command.cpp
    ${ADC_PROJECT_ROOT}/app/src/shell/commands/sensor_rtt_command.cpp
    ${ADC_PROJECT_ROOT}/app/src/shell/commands/status_command.cpp
    ${ADC_PROJECT_ROOT}/app/src/shell/commands/ps_command.cpp
    ${ADC_PROJECT_ROOT}/app/src/telemetry/sensor_rtt_schema_frame_builder.cpp
)
target_link_libraries(adc_unit_tests PRIVATE
    Catch2::Catch2WithMain
    adc_fakeit
    adc_domain
    logging
    io
    sensor_linearization
)
target_include_directories(adc_unit_tests PRIVATE
    ${ADC_PROJECT_ROOT}/app/include
    ${ADC_PROJECT_ROOT}/bsp/include
    ${ADC_PROJECT_ROOT}/os/include
    ${ADC_COMMON_INCLUDE_DIRS}
)
target_compile_definitions(adc_unit_tests PRIVATE UNIT_TESTS=1)
target_compile_features(adc_unit_tests PRIVATE cxx_std_20)

if(PROJECT_IS_TOP_LEVEL)
    add_executable(unit_tests ALIAS adc_unit_tests)
endif()

if(DEFINED catch2_SOURCE_DIR)
    list(APPEND CMAKE_MODULE_PATH "${catch2_SOURCE_DIR}/extras")
endif()
include(Catch)
catch_discover_tests(adc_unit_tests)

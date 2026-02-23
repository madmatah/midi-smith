include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/midismith_firmware_common.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/midismith_require_lib.cmake)

set(ADC_FIRMWARE_TARGET "adc-board")
set(ADC_TARGET_PREFIX "adc")
set(ADC_PROJECT_ROOT "${CMAKE_CURRENT_LIST_DIR}")

project(${ADC_FIRMWARE_TARGET})
message("Build type: " ${CMAKE_BUILD_TYPE})

set(ADC_SEGGER_ROOT "${ADC_PROJECT_ROOT}/../../third_party/SEGGER")

midismith_require_lib(dsp)
midismith_require_lib(midi)
midismith_require_lib(piano-sensing)
midismith_require_lib(logging)
midismith_require_lib(bsp)
midismith_require_lib(io)
midismith_require_lib(os)
midismith_require_lib(shell)
midismith_require_lib(shell-cmd-config)
midismith_require_lib(sensor-linearization)
midismith_require_lib(config)

if(HOST_TESTS)
    enable_testing()

    add_library(adc_domain STATIC)
    target_sources(adc_domain PRIVATE
        domain/src/config/adc_board_config.cpp
        domain/src/shell/commands/version_command.cpp
    )
    target_include_directories(adc_domain PUBLIC
        domain/include
    )
    target_link_libraries(adc_domain PUBLIC dsp midi piano_sensing bsp io os shell shell_cmd_config config)

    add_library(adc_app_lib STATIC)
    target_sources(adc_app_lib PRIVATE
        app/src/storage/adc_board_persistent_configuration.cpp
        app/src/shell/commands/sensor_rtt_command.cpp
        app/src/shell/commands/status_command.cpp
        app/src/shell/commands/ps_command.cpp
        app/src/telemetry/sensor_rtt_schema_frame_builder.cpp
    )
    target_include_directories(adc_app_lib PUBLIC
        app/include
        bsp/include
    )
    target_link_libraries(adc_app_lib PRIVATE
        adc_domain
        bsp
        logging
        io
        shell_cmd_config
        piano_sensing
        sensor_linearization
        config
        os
    )

    if(PROJECT_IS_TOP_LEVEL)
        add_library(domain ALIAS adc_domain)
        add_library(app_lib ALIAS adc_app_lib)
    endif()

    add_subdirectory(tests)
else()

# Create an executable object type
add_executable(${ADC_FIRMWARE_TARGET})

# Versioning
include(cmake/version/versioning.cmake)

# Add compile options for embedded C++
target_compile_options(${ADC_FIRMWARE_TARGET} PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions -fno-rtti>
)

# Add STM32CubeMX generated sources
# CubeMX uses ${CMAKE_PROJECT_NAME} to refer to the firmware target, but when built from the
# monorepo root, CMAKE_PROJECT_NAME resolves to the root project name ("midi-smith-monorepo").
# Shadowing it here ensures the CubeMX-generated CMakeLists.txt targets the correct executable,
# regardless of whether the package is built standalone or from the monorepo root.
set(CMAKE_PROJECT_NAME "${ADC_FIRMWARE_TARGET}")
add_subdirectory(cmake/stm32cubemx)

# Add sources to executable
target_sources(${ADC_FIRMWARE_TARGET} PRIVATE
    # Add user sources here
    app/src/application.cpp
    app/src/app_entry.cpp
    app/src/version.cpp
    app/src/composition/analog_subsystem.cpp
    app/src/composition/config_subsystem.cpp
    app/src/composition/shell_subsystem.cpp
    app/src/composition/sensor_rtt_telemetry_subsystem.cpp
    app/src/storage/adc_board_persistent_configuration.cpp
    app/src/tasks/shell_task.cpp
    app/src/tasks/analog_acquisition_task.cpp
    app/src/tasks/sensor_rtt_telemetry_task.cpp
    app/src/telemetry/sensor_rtt_schema_frame_builder.cpp
    domain/src/config/adc_board_config.cpp
    domain/src/shell/commands/version_command.cpp
    bsp/src/board.cpp
    bsp/src/flash/internal_storage.cpp
    bsp/src/cortex/axi_sram_nocache_mpu.cpp
    bsp/src/cortex/d3_sram_nocache_mpu.cpp
    bsp/src/cortex/flash_config_nocache_mpu.cpp
    bsp/src/adc/adc_dma.cpp
    bsp/src/adc/adc_dma_callbacks.cpp
    bsp/src/adc/adc_trigger_schedule.cpp
    bsp/src/time/tim2_timestamp_counter.cpp
    bsp/src/pins.cpp
    bsp/src/serial/uart_stream_registry.cpp
    app/src/shell/commands/adc_command.cpp
    app/src/shell/commands/ps_command.cpp
    app/src/shell/commands/sensor_rtt_command.cpp
    app/src/shell/commands/status_command.cpp
    ${ADC_SEGGER_ROOT}/RTT/RTT/SEGGER_RTT.c
    ${ADC_SEGGER_ROOT}/RTT/RTT/SEGGER_RTT_printf.c
)


# Add include paths
target_include_directories(${ADC_FIRMWARE_TARGET} PRIVATE
    # Add user defined include paths
    app/include
    ${CMAKE_BINARY_DIR}/generated
    bsp/include
    ${ADC_SEGGER_ROOT}/RTT/RTT
    ${ADC_SEGGER_ROOT}/RTT/Config
    domain/include
)

target_compile_definitions(${ADC_FIRMWARE_TARGET} PRIVATE
    SEGGER_RTT_SECTION=\".rtt\"
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# CMSIS-DSP (GCC, Cortex-M7 little-endian, single-precision FPU)
set(CMSIS_DSP_LIB
    "${ADC_PROJECT_ROOT}/Drivers/CMSIS/DSP/Lib/GCC/libarm_cortexM7lfsp_math.a"
)

# Optional (performance / best practices CMSIS-DSP)
target_compile_definitions(${ADC_FIRMWARE_TARGET} PRIVATE
    ARM_MATH_CM7
)

# Add linked libraries
target_link_options(${ADC_FIRMWARE_TARGET} PRIVATE
    -Wl,-Map=${ADC_FIRMWARE_TARGET}.map
)

target_link_libraries(${ADC_FIRMWARE_TARGET}
    stm32cubemx
    dsp
    midi
    piano_sensing
    bsp
    logging
    io
    os
    shell
    shell_cmd_config
    sensor_linearization
    config

    # Add user defined libraries
    "${CMSIS_DSP_LIB}"
)

add_dependencies(${ADC_FIRMWARE_TARGET} version_check)

set(FLASH_LAYOUT_CHECK_SCRIPT "${ADC_PROJECT_ROOT}/cmake/check_flash_layout.cmake")
add_custom_command(TARGET ${ADC_FIRMWARE_TARGET} POST_BUILD
    COMMAND ${CMAKE_COMMAND}
            -DMAP_FILE=${CMAKE_BINARY_DIR}/${ADC_FIRMWARE_TARGET}.map
            -P ${FLASH_LAYOUT_CHECK_SCRIPT}
    COMMENT "Validate flash layout for persistent configuration"
    VERBATIM
)

endif()

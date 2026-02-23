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

set(ADC_FIRMWARE_TARGET "adc-board")
set(ADC_TARGET_PREFIX "adc")
set(ADC_PROJECT_ROOT "${CMAKE_CURRENT_LIST_DIR}")

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${ADC_FIRMWARE_TARGET})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM CXX)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

set(ADC_COMMON_INCLUDE_DIRS "${ADC_PROJECT_ROOT}/../../libs/common/include")
set(ADC_COMMON_SRC_DIRS "${ADC_PROJECT_ROOT}/../../libs/common/src")

set(ADC_SEGGER_ROOT "${ADC_PROJECT_ROOT}/../../third_party/SEGGER")

if(NOT TARGET dsp)
    add_subdirectory(${ADC_PROJECT_ROOT}/../../libs/dsp libs/dsp)
endif()
if(NOT TARGET midi)
    add_subdirectory(${ADC_PROJECT_ROOT}/../../libs/midi libs/midi)
endif()
if(NOT TARGET logging)
    add_subdirectory(${ADC_PROJECT_ROOT}/../../libs/logging libs/logging)
endif()
if(NOT TARGET io)
    add_subdirectory(${ADC_PROJECT_ROOT}/../../libs/io libs/io)
endif()
if(NOT TARGET shell)
    add_subdirectory(${ADC_PROJECT_ROOT}/../../libs/shell libs/shell)
endif()
if(NOT TARGET sensor_linearization)
    add_subdirectory(${ADC_PROJECT_ROOT}/../../libs/sensor-linearization libs/sensor-linearization)
endif()
if(NOT TARGET config)
    add_subdirectory(${ADC_PROJECT_ROOT}/../../libs/config libs/config)
endif()

if(HOST_TESTS)
    enable_testing()

    add_library(adc_domain STATIC)
    target_sources(adc_domain PRIVATE
        domain/src/config/adc_board_config.cpp
        domain/src/shell/commands/config_command.cpp
        domain/src/shell/commands/version_command.cpp
    )
    target_include_directories(adc_domain PUBLIC
        domain/include
        ${ADC_COMMON_INCLUDE_DIRS}
    )
    target_link_libraries(adc_domain PUBLIC dsp midi io shell config)

    if(PROJECT_IS_TOP_LEVEL)
        add_library(domain ALIAS adc_domain)
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

# Link directories setup
target_link_directories(${ADC_FIRMWARE_TARGET} PRIVATE
    # Add user defined library search paths
)

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
    ${ADC_COMMON_SRC_DIRS}/bsp/gpio.cpp
    bsp/src/pins.cpp
    ${ADC_COMMON_SRC_DIRS}/bsp/rtt_logger.cpp
    bsp/src/rtt_telemetry_sender.cpp
    bsp/src/serial/uart_stream_registry.cpp
    app/src/shell/commands/adc_command.cpp
    domain/src/shell/commands/config_command.cpp
    app/src/shell/commands/ps_command.cpp
    app/src/shell/commands/sensor_rtt_command.cpp
    app/src/shell/commands/status_command.cpp
    ${ADC_SEGGER_ROOT}/RTT/RTT/SEGGER_RTT.c
    ${ADC_SEGGER_ROOT}/RTT/RTT/SEGGER_RTT_printf.c
    ${ADC_COMMON_SRC_DIRS}/os/clock.cpp
    ${ADC_COMMON_SRC_DIRS}/os/task.cpp
    ${ADC_COMMON_SRC_DIRS}/os/queue.cpp
    ${ADC_COMMON_SRC_DIRS}/os/binary_semaphore.cpp
    ${ADC_COMMON_SRC_DIRS}/os/memory.cpp
    os/src/runtime_stats.cpp
    ${ADC_COMMON_SRC_DIRS}/os/this_task.cpp
)


# Add include paths
target_include_directories(${ADC_FIRMWARE_TARGET} PRIVATE
    # Add user defined include paths
    app/include
    ${CMAKE_BINARY_DIR}/generated
    bsp/include
    ${ADC_SEGGER_ROOT}/RTT/RTT
    ${ADC_SEGGER_ROOT}/RTT/Config
    os/include
    domain/include
    ${ADC_COMMON_INCLUDE_DIRS}
)

target_compile_definitions(${ADC_FIRMWARE_TARGET} PRIVATE
    SEGGER_RTT_SECTION=\".rtt\"
)

# Add project symbols (macros)
target_compile_definitions(${ADC_FIRMWARE_TARGET} PRIVATE
    # Add user defined symbols
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
    logging
    io
    shell
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

# Update build/current symlink after each preset change
if(PROJECT_IS_TOP_LEVEL AND CMAKE_EXPORT_COMPILE_COMMANDS)
    file(MAKE_DIRECTORY "${CMAKE_SOURCE_DIR}/build")
    message(STATUS "Updating build/current symlink to : ${CMAKE_BINARY_DIR}")
    execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink
        "${CMAKE_BINARY_DIR}"
        "${CMAKE_SOURCE_DIR}/build/current")
endif()

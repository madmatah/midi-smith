include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/midismith_add_infra_lib.cmake)

set(_SEGGER_RTT_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../third_party/SEGGER/RTT")

if(NOT HOST_TESTS)
    add_library(segger_rtt INTERFACE)
    target_sources(segger_rtt INTERFACE
        ${_SEGGER_RTT_ROOT}/RTT/SEGGER_RTT.c
        ${_SEGGER_RTT_ROOT}/RTT/SEGGER_RTT_printf.c
    )
    target_include_directories(segger_rtt INTERFACE
        ${_SEGGER_RTT_ROOT}/RTT
        ${_SEGGER_RTT_ROOT}/Config
    )
    target_compile_definitions(segger_rtt INTERFACE
        SEGGER_RTT_SECTION=\".rtt\"
    )
endif()

midismith_add_infra_lib(NAME bsp
    SOURCES
        src/gpio.cpp
        src/rtt_logger.cpp
        src/rtt_telemetry_sender.cpp
        src/can/fdcan_transceiver.cpp
    DEPENDS logging
)

if(NOT HOST_TESTS)
    target_link_libraries(bsp INTERFACE segger_rtt)
endif()

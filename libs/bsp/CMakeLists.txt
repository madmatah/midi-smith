include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/midismith_add_infra_lib.cmake)

midismith_add_infra_lib(NAME bsp
    SOURCES
        src/gpio.cpp
        src/rtt_logger.cpp
        src/rtt_telemetry_sender.cpp
    DEPENDS logging
)

add_library(bsp INTERFACE)

target_compile_features(bsp INTERFACE cxx_std_20)
target_include_directories(bsp INTERFACE include)
target_link_libraries(bsp INTERFACE logging)

if(NOT HOST_TESTS)
    target_sources(bsp INTERFACE
        src/gpio.cpp
        src/rtt_logger.cpp
        src/rtt_telemetry_sender.cpp
    )
endif()

include(${CMAKE_CURRENT_LIST_DIR}/../../../cmake/midismith_add_test.cmake)

midismith_add_test(NAME telemetry
    SOURCES telemetry_sender_requirements.test.cpp
)

include(${CMAKE_CURRENT_LIST_DIR}/../../../cmake/midismith_add_test.cmake)

midismith_add_test(NAME midi_monitor
    SOURCES midi_activity_collector.test.cpp
)

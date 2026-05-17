include(${CMAKE_CURRENT_LIST_DIR}/../../../cmake/midismith_add_test.cmake)

midismith_add_test(NAME midi
    SOURCES
        midi_fanout_controller.test.cpp
        midi_input_parser.test.cpp
)

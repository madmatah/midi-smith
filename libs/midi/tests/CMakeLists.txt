include(${CMAKE_CURRENT_LIST_DIR}/../../../cmake/midismith_add_test.cmake)

midismith_add_test(NAME midi
    SOURCES
        message.test.cpp
        midi_fanout_controller.test.cpp
        midi_input_parser.test.cpp
        note_name.test.cpp
)

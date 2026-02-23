include(${CMAKE_CURRENT_LIST_DIR}/../../../cmake/midismith_add_test.cmake)

midismith_add_test(NAME piano_controller
    SOURCES midi_piano.test.cpp
    USE_FAKEIT
)

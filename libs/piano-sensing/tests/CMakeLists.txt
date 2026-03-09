include(${CMAKE_CURRENT_LIST_DIR}/../../../cmake/midismith_add_test.cmake)

midismith_add_test(NAME piano_sensing
    SOURCES
        composite_sensor_event_handler.test.cpp
        midi_velocity_engine.test.cpp
        note_release_detector_stage.test.cpp
        velocity/constant_velocity_mapper.test.cpp
        velocity/linear_velocity_mapper.test.cpp
        velocity/logarithmic_velocity_mapper.test.cpp
        velocity/exponential_velocity_mapper.test.cpp
        velocity/goebl_logarithmic_velocity_mapper.test.cpp
    USE_FAKEIT
)

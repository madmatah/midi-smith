include(${CMAKE_CURRENT_LIST_DIR}/../../../cmake/midismith_add_test.cmake)

midismith_add_test(NAME sensor_linearization
    SOURCES
        sensor_linear_processor.test.cpp
        sensor_response_curve.test.cpp
        lookup_table_generator.test.cpp
)

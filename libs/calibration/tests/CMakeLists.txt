include(${CMAKE_CURRENT_LIST_DIR}/../../../cmake/midismith_add_test.cmake)

midismith_add_test(NAME calibration
    SOURCES
        calibration_segment_packer.test.cpp
)

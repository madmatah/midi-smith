include(${CMAKE_CURRENT_LIST_DIR}/../../../cmake/midismith_add_test.cmake)

midismith_add_test(NAME firmware_staging
    SOURCES
        staging_writer.test.cpp
)

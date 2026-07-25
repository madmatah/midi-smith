include(${CMAKE_CURRENT_LIST_DIR}/../../../cmake/midismith_add_test.cmake)

midismith_add_test(NAME checksum
    SOURCES
        crc32.test.cpp
)

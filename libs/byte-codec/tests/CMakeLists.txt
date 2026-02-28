include(${CMAKE_CURRENT_LIST_DIR}/../../../cmake/midismith_add_test.cmake)

midismith_add_test(NAME byte_codec
    SOURCES
        little_endian.test.cpp
)

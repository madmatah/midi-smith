include(${CMAKE_CURRENT_LIST_DIR}/../../../cmake/midismith_add_test.cmake)

midismith_add_test(NAME protocol_can
    SOURCES
        can_mapper.test.cpp
)

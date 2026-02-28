include(${CMAKE_CURRENT_LIST_DIR}/../../../cmake/midismith_add_test.cmake)

midismith_add_test(NAME can_broker
    SOURCES
        can_task.test.cpp
)

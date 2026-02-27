include(${CMAKE_CURRENT_LIST_DIR}/../../../cmake/midismith_add_test.cmake)

midismith_add_test(NAME protocol
    SOURCES
        main.cpp
        messages.test.cpp
        message_parser.test.cpp
        builders.test.cpp
)

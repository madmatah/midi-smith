include(${CMAKE_CURRENT_LIST_DIR}/../../../cmake/midismith_add_test.cmake)

midismith_add_test(NAME shell
    SOURCES
        command_parser.test.cpp
        command_dispatcher.test.cpp
        line_editor.test.cpp
        shell_engine.test.cpp
)

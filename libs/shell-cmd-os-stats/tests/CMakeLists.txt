include(${CMAKE_CURRENT_LIST_DIR}/../../../cmake/midismith_add_test.cmake)

midismith_add_test(NAME shell_cmd_os_stats
    SOURCES
        ps_command.test.cpp
        status_command.test.cpp
)

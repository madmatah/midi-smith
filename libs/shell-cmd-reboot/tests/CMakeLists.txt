include(${CMAKE_CURRENT_LIST_DIR}/../../../cmake/midismith_add_test.cmake)

midismith_add_test(NAME shell_cmd_reboot
    SOURCES
        reboot_command.test.cpp
)

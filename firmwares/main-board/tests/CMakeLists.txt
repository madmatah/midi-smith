include(${CMAKE_CURRENT_LIST_DIR}/../../../cmake/midismith_add_firmware_test.cmake)

midismith_add_firmware_test(NAME main
    SOURCES app/midi/async_task_midi_controller.test.cpp
    LINK_LIBRARIES main_domain main_app_lib
    USE_FAKEIT
)

include(${CMAKE_CURRENT_LIST_DIR}/../../../cmake/midismith_add_test.cmake)

midismith_add_test(NAME image_installer
    SOURCES
        staged_image_installer.test.cpp
)

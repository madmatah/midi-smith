include(${CMAKE_CURRENT_LIST_DIR}/../../../cmake/midismith_add_test.cmake)

midismith_add_test(NAME firmware_image
    SOURCES
        image_installability.test.cpp
        image_header.test.cpp
        packaged_container.test.cpp
)

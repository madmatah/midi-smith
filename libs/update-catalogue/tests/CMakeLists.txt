include(${CMAKE_CURRENT_LIST_DIR}/../../../cmake/midismith_add_test.cmake)

midismith_add_test(NAME update_catalogue
    SOURCES
        update_catalogue.test.cpp
)

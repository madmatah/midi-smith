include(${CMAKE_CURRENT_LIST_DIR}/../../../cmake/midismith_add_test.cmake)

midismith_add_test(NAME product_id
    SOURCES
        product_id.test.cpp
)

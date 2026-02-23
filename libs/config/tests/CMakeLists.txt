include(${CMAKE_CURRENT_LIST_DIR}/../../../cmake/midismith_add_test.cmake)

midismith_add_test(NAME config
    SOURCES
        crc32.test.cpp
        config_validator.test.cpp
        transactional_config_dictionary.test.cpp
)

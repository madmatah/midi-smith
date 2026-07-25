include(${CMAKE_CURRENT_LIST_DIR}/../../../cmake/midismith_add_test.cmake)

midismith_add_test(NAME config
    SOURCES
        config_validator.test.cpp
        storage_manager.test.cpp
        transactional_config_dictionary.test.cpp
)

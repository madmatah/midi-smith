include(${CMAKE_CURRENT_LIST_DIR}/../../../cmake/midismith_add_test.cmake)

midismith_add_test(NAME os_types
    SOURCES
        runtime_status_stats_provider.test.cpp
    USE_FAKEIT
)

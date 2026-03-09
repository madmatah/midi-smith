include(${CMAKE_CURRENT_LIST_DIR}/../../../cmake/midismith_add_test.cmake)

midismith_add_test(NAME stats
    SOURCES
        stats_provider_requirements.test.cpp
    USE_FAKEIT
)

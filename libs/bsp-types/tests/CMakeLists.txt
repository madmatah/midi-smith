include(${CMAKE_CURRENT_LIST_DIR}/../../../cmake/midismith_add_test.cmake)

midismith_add_test(NAME bsp_types
    SOURCES
        can_bus_stats_provider.test.cpp
)

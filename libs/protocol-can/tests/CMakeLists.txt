include(${CMAKE_CURRENT_LIST_DIR}/../../../cmake/midismith_add_test.cmake)

midismith_add_test(NAME protocol_can
    SOURCES
        can_filter_factory.test.cpp
        can_inbound_decode_stats_provider.test.cpp
        can_mapper.test.cpp
        can_to_protocol_adapter.test.cpp
)

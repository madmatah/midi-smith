include(${CMAKE_CURRENT_LIST_DIR}/../../../cmake/midismith_add_test.cmake)

midismith_add_test(NAME boot_control
    SOURCES
        boot_decision.test.cpp
        boot_journal.test.cpp
        boot_journal_record.test.cpp
        boot_journal_writer.test.cpp
)

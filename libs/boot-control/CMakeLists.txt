cmake_minimum_required(VERSION 3.22)
project(boot_control LANGUAGES CXX)

include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/midismith_add_lib.cmake)

midismith_add_lib(NAME boot_control
    SOURCES
        src/boot_decision.cpp
        src/boot_journal.cpp
        src/boot_journal_record.cpp
        src/boot_journal_writer.cpp
    DEPENDS byte-codec checksum product-id
)

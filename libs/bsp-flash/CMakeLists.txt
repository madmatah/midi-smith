include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/midismith_add_infra_lib.cmake)

midismith_add_infra_lib(NAME bsp_flash
    SOURCES
        src/internal_flash.cpp
        src/journal_storage.cpp
    DEPENDS boot-control flash-layout
)

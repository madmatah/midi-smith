include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/midismith_add_infra_lib.cmake)

midismith_add_infra_lib(NAME os
    SOURCES
        src/binary_semaphore.cpp
        src/clock.cpp
        src/memory.cpp
        src/queue.cpp
        src/task.cpp
        src/this_task.cpp
        src/runtime_stats.cpp
    DEPENDS os-types
)

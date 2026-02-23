add_library(os INTERFACE)

target_compile_features(os INTERFACE cxx_std_20)
target_include_directories(os INTERFACE include)

if(NOT HOST_TESTS)
    target_sources(os INTERFACE
        src/binary_semaphore.cpp
        src/clock.cpp
        src/memory.cpp
        src/queue.cpp
        src/task.cpp
        src/this_task.cpp
        src/runtime_stats.cpp
    )
endif()

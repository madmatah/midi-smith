include_guard(GLOBAL)
include(FetchContent)

FetchContent_Declare(fakeit_dep
    GIT_REPOSITORY https://github.com/eranpeer/FakeIt.git
    GIT_TAG 2.5.0
)
FetchContent_GetProperties(fakeit_dep)
if(NOT fakeit_dep_POPULATED)
    FetchContent_Populate(fakeit_dep)
endif()

add_library(fakeit INTERFACE)
target_include_directories(fakeit INTERFACE
    ${fakeit_dep_SOURCE_DIR}/single_header/standalone
)

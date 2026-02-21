include(FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.5.4
)
FetchContent_MakeAvailable(Catch2)

FetchContent_Declare(
    main_fakeit_dep
    GIT_REPOSITORY https://github.com/eranpeer/FakeIt.git
    GIT_TAG 2.5.0
)
FetchContent_GetProperties(main_fakeit_dep)
if(NOT main_fakeit_dep_POPULATED)
    FetchContent_Populate(main_fakeit_dep)
endif()

add_library(master_fakeit INTERFACE)
target_include_directories(master_fakeit INTERFACE
    ${main_fakeit_dep_SOURCE_DIR}/single_header/standalone
)

add_executable(main_unit_tests
    test_sanity.cpp
    domain/music/piano/midi_piano.test.cpp
    app/midi/async_task_midi_controller.test.cpp
)
target_link_libraries(main_unit_tests PRIVATE
    Catch2::Catch2WithMain
    master_fakeit
    main_domain
    main_app_lib
)
target_compile_features(main_unit_tests PRIVATE cxx_std_20)

if(PROJECT_IS_TOP_LEVEL)
    add_executable(unit_tests ALIAS main_unit_tests)
endif()

if(DEFINED catch2_SOURCE_DIR)
    list(APPEND CMAKE_MODULE_PATH "${catch2_SOURCE_DIR}/extras")
endif()
include(Catch)
catch_discover_tests(main_unit_tests)

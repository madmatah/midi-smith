include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.5.4
)
FetchContent_MakeAvailable(Catch2)

FetchContent_Declare(
  piano_controller_fakeit_dep
  GIT_REPOSITORY https://github.com/eranpeer/FakeIt.git
  GIT_TAG 2.5.0
)
FetchContent_GetProperties(piano_controller_fakeit_dep)
if(NOT piano_controller_fakeit_dep_POPULATED)
  FetchContent_Populate(piano_controller_fakeit_dep)
endif()

add_library(piano_controller_fakeit INTERFACE)
target_include_directories(piano_controller_fakeit INTERFACE
  ${piano_controller_fakeit_dep_SOURCE_DIR}/single_header/standalone
)

add_executable(piano_controller_unit_tests
  midi_piano.test.cpp
)
target_link_libraries(piano_controller_unit_tests PRIVATE
  Catch2::Catch2WithMain
  piano_controller_fakeit
  piano_controller
)
target_compile_features(piano_controller_unit_tests PRIVATE cxx_std_20)

if(DEFINED catch2_SOURCE_DIR)
  list(APPEND CMAKE_MODULE_PATH "${catch2_SOURCE_DIR}/extras")
endif()
include(Catch)
catch_discover_tests(piano_controller_unit_tests)

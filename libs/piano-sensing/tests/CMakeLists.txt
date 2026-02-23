include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.5.4
)
FetchContent_MakeAvailable(Catch2)

add_executable(piano_sensing_unit_tests
  midi_velocity_engine.test.cpp
  note_release_detector_stage.test.cpp
  velocity/constant_velocity_mapper.test.cpp
  velocity/linear_velocity_mapper.test.cpp
  velocity/logarithmic_velocity_mapper.test.cpp
  velocity/exponential_velocity_mapper.test.cpp
  velocity/goebl_logarithmic_velocity_mapper.test.cpp
)
target_link_libraries(piano_sensing_unit_tests PRIVATE Catch2::Catch2WithMain piano_sensing)
target_compile_definitions(piano_sensing_unit_tests PRIVATE UNIT_TESTS=1)
target_compile_features(piano_sensing_unit_tests PRIVATE cxx_std_20)

if(DEFINED catch2_SOURCE_DIR)
  list(APPEND CMAKE_MODULE_PATH "${catch2_SOURCE_DIR}/extras")
endif()
include(Catch)
catch_discover_tests(piano_sensing_unit_tests)

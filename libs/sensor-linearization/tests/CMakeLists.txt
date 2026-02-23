include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.5.4
)
FetchContent_MakeAvailable(Catch2)

add_executable(sensor_linearization_unit_tests
  sensor_linear_processor.test.cpp
  sensor_response_curve.test.cpp
)
target_link_libraries(sensor_linearization_unit_tests PRIVATE Catch2::Catch2WithMain sensor_linearization)
target_compile_definitions(sensor_linearization_unit_tests PRIVATE UNIT_TESTS=1)
target_compile_features(sensor_linearization_unit_tests PRIVATE cxx_std_20)

if(DEFINED catch2_SOURCE_DIR)
  list(APPEND CMAKE_MODULE_PATH "${catch2_SOURCE_DIR}/extras")
endif()
include(Catch)
catch_discover_tests(sensor_linearization_unit_tests)

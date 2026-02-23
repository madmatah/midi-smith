include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.5.4
)
FetchContent_MakeAvailable(Catch2)

add_executable(config_unit_tests
  crc32.test.cpp
  config_validator.test.cpp
  transactional_config_dictionary.test.cpp
)
target_link_libraries(config_unit_tests PRIVATE Catch2::Catch2WithMain config)
target_compile_definitions(config_unit_tests PRIVATE UNIT_TESTS=1)
target_compile_features(config_unit_tests PRIVATE cxx_std_20)

if(DEFINED catch2_SOURCE_DIR)
  list(APPEND CMAKE_MODULE_PATH "${catch2_SOURCE_DIR}/extras")
endif()
include(Catch)
catch_discover_tests(config_unit_tests)

include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.5.4
)
FetchContent_MakeAvailable(Catch2)

add_executable(shell_cmd_config_unit_tests
  config_command.test.cpp
)
target_link_libraries(shell_cmd_config_unit_tests PRIVATE Catch2::Catch2WithMain shell_cmd_config)
target_compile_features(shell_cmd_config_unit_tests PRIVATE cxx_std_20)

if(DEFINED catch2_SOURCE_DIR)
  list(APPEND CMAKE_MODULE_PATH "${catch2_SOURCE_DIR}/extras")
endif()
include(Catch)
catch_discover_tests(shell_cmd_config_unit_tests)

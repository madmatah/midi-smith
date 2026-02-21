cmake_minimum_required(VERSION 3.22)

if(NOT DEFINED ADC_PROJECT_ROOT)
  get_filename_component(ADC_PROJECT_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
endif()

set(VERSION_GENERATED_DIR "${CMAKE_BINARY_DIR}/generated")
set(VERSION_GENERATED_HEADER "${VERSION_GENERATED_DIR}/app/version_build.hpp")
set(VERSION_SCRIPT "${ADC_PROJECT_ROOT}/cmake/version/generate_version_header.cmake")

if(CMAKE_CONFIGURATION_TYPES)
  set(VERSION_BUILD_TYPE_ARG "$<CONFIG>")
else()
  set(VERSION_BUILD_TYPE_ARG "${CMAKE_BUILD_TYPE}")
endif()

# We want to run the version script on every build to check if the git state changed.
# To force Ninja/Make to always run the command, we make it produce a
# non-existent file in addition to the header.
add_custom_command(
  OUTPUT "${VERSION_GENERATED_HEADER}" "FORCE_VERSION_CHECK"
  COMMAND "${CMAKE_COMMAND}"
          -DOUTPUT_HEADER=${VERSION_GENERATED_HEADER}
          -DSOURCE_DIR=${ADC_PROJECT_ROOT}
          -DVERSION_BUILD_TYPE=${VERSION_BUILD_TYPE_ARG}
          -P ${VERSION_SCRIPT}
  DEPENDS ${VERSION_SCRIPT}
  COMMENT "Checking for firmware version updates..."
  VERBATIM
)

# This target is ALL, so it runs on every build.
# It depends on the header (so the command runs).
add_custom_target(
  version_check ALL
  DEPENDS "${VERSION_GENERATED_HEADER}"
)

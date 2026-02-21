cmake_minimum_required(VERSION 3.22)

if(NOT DEFINED OUTPUT_HEADER)
  message(FATAL_ERROR "OUTPUT_HEADER is required")
endif()

if(NOT DEFINED SOURCE_DIR)
  get_filename_component(SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
endif()
get_filename_component(SOURCE_DIR "${SOURCE_DIR}" ABSOLUTE)

if(NOT DEFINED VERSION_BUILD_TYPE)
  set(VERSION_BUILD_TYPE "unknown")
endif()

find_program(GIT_EXECUTABLE git)

set(full_version "unknown")
set(commit_date "unknown")

if(GIT_EXECUTABLE)
  execute_process(
    COMMAND "${GIT_EXECUTABLE}" -C "${SOURCE_DIR}" rev-parse --is-inside-work-tree
    RESULT_VARIABLE git_is_repo_result
    OUTPUT_QUIET
    ERROR_QUIET
  )

  if(git_is_repo_result EQUAL 0)
    execute_process(
      COMMAND "${GIT_EXECUTABLE}" -C "${SOURCE_DIR}" describe --tags --always --dirty --abbrev=10
      RESULT_VARIABLE git_describe_result
      OUTPUT_VARIABLE git_describe_out
      ERROR_QUIET
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(git_describe_result EQUAL 0 AND NOT git_describe_out STREQUAL "")
      set(full_version "${git_describe_out}")
    endif()

    execute_process(
      COMMAND "${GIT_EXECUTABLE}" -C "${SOURCE_DIR}" show -s --format=%cI HEAD
      RESULT_VARIABLE git_show_result
      OUTPUT_VARIABLE git_show_out
      ERROR_QUIET
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(git_show_result EQUAL 0 AND NOT git_show_out STREQUAL "")
      set(commit_date "${git_show_out}")
    endif()
  endif()
endif()

string(REPLACE "\\" "\\\\" full_version_escaped "${full_version}")
string(REPLACE "\"" "\\\"" full_version_escaped "${full_version_escaped}")
string(REPLACE "\\" "\\\\" commit_date_escaped "${commit_date}")
string(REPLACE "\"" "\\\"" commit_date_escaped "${commit_date_escaped}")
string(REPLACE "\\" "\\\\" build_type_escaped "${VERSION_BUILD_TYPE}")
string(REPLACE "\"" "\\\"" build_type_escaped "${build_type_escaped}")

get_filename_component(output_dir "${OUTPUT_HEADER}" DIRECTORY)
file(MAKE_DIRECTORY "${output_dir}")

set(header_contents
"#pragma once

#define APP_VERSION_FULL \"${full_version_escaped}\"
#define APP_VERSION_COMMIT_DATE \"${commit_date_escaped}\"
#define APP_VERSION_BUILD_TYPE \"${build_type_escaped}\"
")

set(tmp_header "${OUTPUT_HEADER}.tmp")
file(WRITE "${tmp_header}" "${header_contents}")
execute_process(COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${tmp_header}" "${OUTPUT_HEADER}")
file(REMOVE "${tmp_header}")


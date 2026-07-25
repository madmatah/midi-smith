foreach(required_variable MAP_FILE EXPECTED_CONFIG_ORIGIN EXPECTED_CONFIG_SIZE
                          EXPECTED_APPLICATION_ORIGIN EXPECTED_APPLICATION_SIZE)
  if(NOT DEFINED ${required_variable})
    message(FATAL_ERROR "${required_variable} must be provided to check_flash_layout.cmake")
  endif()
endforeach()

if(NOT EXISTS "${MAP_FILE}")
  message(FATAL_ERROR "Map file not found: ${MAP_FILE}")
endif()

file(READ "${MAP_FILE}" map_contents)

function(extract_output_section section_name out_address out_size)
  string(REGEX MATCH "\n\\.${section_name}[ \t]+0x([0-9A-Fa-f]+)[ \t]+0x([0-9A-Fa-f]+)"
                     section_match "\n${map_contents}")
  if(NOT section_match)
    message(FATAL_ERROR "Unable to find output section .${section_name} in map file: ${MAP_FILE}")
  endif()
  set(${out_address} "0x${CMAKE_MATCH_1}" PARENT_SCOPE)
  set(${out_size} "0x${CMAKE_MATCH_2}" PARENT_SCOPE)
endfunction()

function(extract_memory_region region_name out_origin out_size)
  string(REGEX MATCH "\n${region_name}[ \t]+0x([0-9A-Fa-f]+)[ \t]+0x([0-9A-Fa-f]+)"
                     region_match "\n${map_contents}")
  if(NOT region_match)
    message(FATAL_ERROR "Unable to find memory region ${region_name} in map file: ${MAP_FILE}")
  endif()
  set(${out_origin} "0x${CMAKE_MATCH_1}" PARENT_SCOPE)
  set(${out_size} "0x${CMAKE_MATCH_2}" PARENT_SCOPE)
endfunction()

function(require_equal what actual expected)
  math(EXPR actual_value "${actual}")
  math(EXPR expected_value "${expected}")
  if(NOT actual_value EQUAL expected_value)
    message(FATAL_ERROR "${what} mismatch: expected ${expected}, got ${actual}")
  endif()
endfunction()

extract_memory_region("FLASH_CONFIG" flash_config_origin flash_config_region_size)
extract_memory_region("FLASH" flash_origin flash_region_size)
extract_output_section("flash_config" flash_config_start flash_config_size)
extract_output_section("isr_vector" isr_vector_start isr_vector_size)
extract_output_section("text" text_start text_size)
extract_output_section("rodata" rodata_start rodata_size)

require_equal("FLASH_CONFIG origin" "${flash_config_origin}" "${EXPECTED_CONFIG_ORIGIN}")
require_equal("FLASH_CONFIG size" "${flash_config_region_size}" "${EXPECTED_CONFIG_SIZE}")
require_equal(".flash_config start" "${flash_config_start}" "${EXPECTED_CONFIG_ORIGIN}")
require_equal("FLASH origin" "${flash_origin}" "${EXPECTED_APPLICATION_ORIGIN}")
require_equal("FLASH size" "${flash_region_size}" "${EXPECTED_APPLICATION_SIZE}")
require_equal(".isr_vector start" "${isr_vector_start}" "${EXPECTED_APPLICATION_ORIGIN}")

math(EXPR flash_config_size_value "${flash_config_size}")
math(EXPR expected_config_size_value "${EXPECTED_CONFIG_SIZE}")
if(flash_config_size_value GREATER expected_config_size_value)
  message(FATAL_ERROR ".flash_config size exceeds FLASH_CONFIG region: ${flash_config_size}")
endif()

math(EXPR application_origin_value "${EXPECTED_APPLICATION_ORIGIN}")
math(EXPR application_size_value "${EXPECTED_APPLICATION_SIZE}")
math(EXPR application_end_value "${application_origin_value} + ${application_size_value}")

foreach(section_name text rodata)
  math(EXPR section_start_value "${${section_name}_start}")
  math(EXPR section_size_value "${${section_name}_size}")
  math(EXPR section_end_value "${section_start_value} + ${section_size_value}")
  if(section_start_value LESS application_origin_value OR
     section_end_value GREATER application_end_value)
    math(EXPR section_end_hex "${section_end_value}" OUTPUT_FORMAT HEXADECIMAL)
    message(FATAL_ERROR
            ".${section_name} escapes the application slot: "
            "${${section_name}_start}..${section_end_hex}")
  endif()
endforeach()

message(STATUS
        "Flash layout validated: application at ${EXPECTED_APPLICATION_ORIGIN}, "
        "configuration at ${EXPECTED_CONFIG_ORIGIN}")

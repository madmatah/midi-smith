if(NOT DEFINED MAP_FILE)
  message(FATAL_ERROR "MAP_FILE must be provided to check_flash_layout.cmake")
endif()

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

string(REGEX MATCH "\nFLASH_CONFIG[ \t]+0x([0-9A-Fa-f]+)[ \t]+0x([0-9A-Fa-f]+)[ \t]+r"
                   flash_config_region_match "\n${map_contents}")
if(NOT flash_config_region_match)
  message(FATAL_ERROR "Unable to find FLASH_CONFIG memory region in map file: ${MAP_FILE}")
endif()

set(flash_config_region_origin "0x${CMAKE_MATCH_1}")
set(flash_config_region_size "0x${CMAKE_MATCH_2}")

extract_output_section("text" text_start text_size)
extract_output_section("rodata" rodata_start rodata_size)
extract_output_section("flash_config" flash_config_start flash_config_size)

set(expected_flash_config_origin 0x081E0000)
set(expected_flash_config_size 0x00020000)

math(EXPR flash_config_region_origin_value "${flash_config_region_origin}")
math(EXPR flash_config_region_size_value "${flash_config_region_size}")
math(EXPR flash_config_start_value "${flash_config_start}")
math(EXPR flash_config_size_value "${flash_config_size}")
math(EXPR text_start_value "${text_start}")
math(EXPR text_size_value "${text_size}")
math(EXPR rodata_start_value "${rodata_start}")
math(EXPR rodata_size_value "${rodata_size}")
math(EXPR expected_flash_config_origin_value "${expected_flash_config_origin}")
math(EXPR expected_flash_config_size_value "${expected_flash_config_size}")

if(NOT flash_config_region_origin_value EQUAL expected_flash_config_origin_value)
  message(FATAL_ERROR
          "FLASH_CONFIG origin mismatch: expected 0x081E0000, got ${flash_config_region_origin}")
endif()

if(NOT flash_config_region_size_value EQUAL expected_flash_config_size_value)
  message(FATAL_ERROR
          "FLASH_CONFIG size mismatch: expected 0x00020000, got ${flash_config_region_size}")
endif()

if(NOT flash_config_start_value EQUAL expected_flash_config_origin_value)
  message(FATAL_ERROR
          ".flash_config start mismatch: expected 0x081E0000, got ${flash_config_start}")
endif()

if(flash_config_size_value GREATER expected_flash_config_size_value)
  message(FATAL_ERROR
          ".flash_config size exceeds FLASH_CONFIG region: ${flash_config_size} > 0x00020000")
endif()

math(EXPR text_end_value "${text_start_value} + ${text_size_value}")
math(EXPR rodata_end_value "${rodata_start_value} + ${rodata_size_value}")
math(EXPR text_end_hex "${text_end_value}" OUTPUT_FORMAT HEXADECIMAL)
math(EXPR rodata_end_hex "${rodata_end_value}" OUTPUT_FORMAT HEXADECIMAL)

if(text_end_value GREATER expected_flash_config_origin_value)
  message(FATAL_ERROR
          ".text overlaps FLASH_CONFIG region: end=${text_end_hex}, start=0x081E0000")
endif()

if(rodata_end_value GREATER expected_flash_config_origin_value)
  message(FATAL_ERROR
          ".rodata overlaps FLASH_CONFIG region: end=${rodata_end_hex}, start=0x081E0000")
endif()

message(STATUS
        "Flash layout validated: FLASH_CONFIG at 0x081E0000, no overlap with .text/.rodata")

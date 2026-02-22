cmake_minimum_required(VERSION 3.22)

project(midi-smith-monorepo LANGUAGES NONE)

option(HOST_TESTS "Build host-only tests for all firmwares" OFF)
set(MIDISMITH_ACTIVE_FIRMWARE "adc" CACHE STRING "Firmware to build when HOST_TESTS=OFF")
set_property(CACHE MIDISMITH_ACTIVE_FIRMWARE PROPERTY STRINGS adc master)

# Enable compile_commands.json at monorepo level for clangd.
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

if(HOST_TESTS)
    enable_testing()
    add_subdirectory(firmwares/adc-board)
    add_subdirectory(firmwares/main-board)
else()
    if(MIDISMITH_ACTIVE_FIRMWARE STREQUAL "adc")
        add_subdirectory(firmwares/adc-board)
    elseif(MIDISMITH_ACTIVE_FIRMWARE STREQUAL "master")
        add_subdirectory(firmwares/main-board)
    else()
        message(FATAL_ERROR
            "MIDISMITH_ACTIVE_FIRMWARE must be one of: adc, master. Got: ${MIDISMITH_ACTIVE_FIRMWARE}")
    endif()
endif()

# Monorepo-wide lint, format and format_check targets.
# Covers all project-owned code across both packages and common.
# cpplint reads CPPLINT.cfg by walking up from each file — the root CPPLINT.cfg applies to all.
# clang-format respects .clang-format-ignore for IDE/local tooling; in CI, the explicit file list
# produced by this GLOB is the authoritative scope (same list for lint and format).
file(GLOB_RECURSE MONOREPO_SOURCE_FILES CONFIGURE_DEPENDS
    "firmwares/adc-board/app/*.*"    "firmwares/adc-board/bsp/*.*"
    "firmwares/adc-board/os/*.*"     "firmwares/adc-board/domain/*.*"
    "firmwares/adc-board/tests/*.*"
    "firmwares/main-board/app/*.*"   "firmwares/main-board/bsp/*.*"
    "firmwares/main-board/os/*.*"    "firmwares/main-board/domain/*.*"
    "firmwares/main-board/tests/*.*"
    "libs/common/include/*.*"        "libs/common/src/*.*"
)
set(FILES_TO_CHECK "")
foreach(FILE_PATH ${MONOREPO_SOURCE_FILES})
    if(FILE_PATH MATCHES "\\.(cpp|hpp|h)$")
        list(APPEND FILES_TO_CHECK ${FILE_PATH})
    endif()
endforeach()

add_custom_target(lint
    COMMAND cpplint --quiet ${FILES_TO_CHECK}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Lint (monorepo)"
)
add_custom_target(format
    COMMAND clang-format -i ${FILES_TO_CHECK}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Format (monorepo)"
)
add_custom_target(format_check
    COMMAND clang-format --dry-run --Werror ${FILES_TO_CHECK}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Format check (monorepo, dry-run)"
)

# Keep a stable compilation database location for clangd.
if(CMAKE_EXPORT_COMPILE_COMMANDS)
    file(MAKE_DIRECTORY "${CMAKE_SOURCE_DIR}/build")
    message(STATUS "Updating build/current symlink to: ${CMAKE_BINARY_DIR}")
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E create_symlink
            "${CMAKE_BINARY_DIR}"
            "${CMAKE_SOURCE_DIR}/build/current"
    )
endif()

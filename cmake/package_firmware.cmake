cmake_minimum_required(VERSION 3.22)

foreach(required_variable
        ELF_FILE BIN_FILE MSFW_FILE OBJCOPY PACKAGER PRODUCT
        LOAD_ADDRESS MAXIMUM_PAYLOAD_SIZE_BYTES MIN_PROTOCOL_VERSION VERSION_CMAKE_FILE)
    if(NOT DEFINED ${required_variable})
        message(FATAL_ERROR "${required_variable} must be provided to package_firmware.cmake")
    endif()
endforeach()

if(NOT EXISTS "${ELF_FILE}")
    message(FATAL_ERROR "ELF not found: ${ELF_FILE}")
endif()

set(firmware_version "unknown")
set(firmware_commit_date "unknown")
if(EXISTS "${VERSION_CMAKE_FILE}")
    include("${VERSION_CMAKE_FILE}")
    set(firmware_version "${MIDISMITH_VERSION_FULL}")
    set(firmware_commit_date "${MIDISMITH_VERSION_COMMIT_DATE}")
endif()

execute_process(
    COMMAND "${OBJCOPY}" -O binary "${ELF_FILE}" "${BIN_FILE}"
    RESULT_VARIABLE objcopy_result
)
if(NOT objcopy_result EQUAL 0)
    message(FATAL_ERROR "objcopy failed for ${ELF_FILE}")
endif()

find_program(PYTHON_EXECUTABLE NAMES python3 python REQUIRED)

execute_process(
    COMMAND "${PYTHON_EXECUTABLE}" "${PACKAGER}" pack
            --input "${BIN_FILE}"
            --elf "${ELF_FILE}"
            --output "${MSFW_FILE}"
            --product "${PRODUCT}"
            --load-address "${LOAD_ADDRESS}"
            --maximum-payload-size-bytes "${MAXIMUM_PAYLOAD_SIZE_BYTES}"
            --min-protocol-version "${MIN_PROTOCOL_VERSION}"
            --version "${firmware_version}"
            --build-date "${firmware_commit_date}"
    RESULT_VARIABLE packager_result
)
if(NOT packager_result EQUAL 0)
    message(FATAL_ERROR "Packaging failed for ${ELF_FILE}")
endif()

include_guard(GLOBAL)

include(${CMAKE_CURRENT_LIST_DIR}/midismith_flash_layout.cmake)

# Produces the field-updatable .msfw container next to the firmware ELF. The container is what
# lands on the SD card and what travels over CAN; the ELF stays the STLINK/debug artefact.
function(midismith_firmware_package TARGET)
    cmake_parse_arguments(_MSPKG "" "PRODUCT" "" ${ARGN})

    if(NOT _MSPKG_PRODUCT)
        message(FATAL_ERROR "midismith_firmware_package requires PRODUCT")
    endif()

    set(PACKAGE_SCRIPT "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/package_firmware.cmake")
    set(PACKAGER_TOOL "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../tools/firmware_packager.py")
    set(VERSION_CMAKE_FILE "${CMAKE_BINARY_DIR}/generated/version_build.cmake")

    add_custom_command(TARGET ${TARGET} POST_BUILD
        COMMAND ${CMAKE_COMMAND}
                -DELF_FILE=$<TARGET_FILE:${TARGET}>
                -DBIN_FILE=$<TARGET_FILE_DIR:${TARGET}>/${_MSPKG_PRODUCT}.bin
                -DMSFW_FILE=$<TARGET_FILE_DIR:${TARGET}>/${_MSPKG_PRODUCT}.msfw
                -DOBJCOPY=${CMAKE_OBJCOPY}
                -DPACKAGER=${PACKAGER_TOOL}
                -DPRODUCT=${_MSPKG_PRODUCT}
                -DLOAD_ADDRESS=${MIDISMITH_APPLICATION_LOAD_ADDRESS}
                -DMAXIMUM_PAYLOAD_SIZE_BYTES=${MIDISMITH_APPLICATION_SLOT_SIZE_BYTES}
                -DMIN_PROTOCOL_VERSION=${MIDISMITH_MIN_COMPATIBLE_PROTOCOL_VERSION}
                -DVERSION_CMAKE_FILE=${VERSION_CMAKE_FILE}
                -P ${PACKAGE_SCRIPT}
        COMMENT "Packaging ${_MSPKG_PRODUCT}.msfw"
        VERBATIM
    )
endfunction()

include_guard(GLOBAL)

# The flash map both STM32H743 firmware packages share. They carry the same part and are installed
# by the same bootloader, so the map is a monorepo level fact rather than a per-package one.
#
# libs/flash-layout turns these values into C++ constants; the linker scripts are checked against
# them at build time. Nothing else may restate an address.
#
# Every region an application writes lives in the bank the application does NOT execute from.
# An H743 stalls the CPU for the whole duration of an erase on the bank it is fetching from, so a
# same bank write would freeze FreeRTOS, the CAN bus and the MIDI path for seconds.
#
#   BANK 1  0x0800_0000 .. 0x080F_FFFF   bootloader and every region the application writes
#     S0    0x0800_0000  128K  bootloader
#     S1-S3 0x0802_0000  384K  staging slot
#     S4-S5 0x0808_0000  256K  free
#     S6    0x080C_0000  128K  boot journal
#     S7    0x080E_0000  128K  application configuration
#
#   BANK 2  0x0810_0000 .. 0x081F_FFFF   the application alone
#     S0-S2 0x0810_0000  384K  application slot
#     S3-S5 0x0816_0000  384K  reserved for the rollback image
#     S6-S7 0x081C_0000  256K  free

set(MIDISMITH_FLASH_SECTOR_SIZE_BYTES 131072 CACHE INTERNAL "midi-smith flash map")
set(MIDISMITH_FLASH_BANK_SIZE_BYTES 1048576 CACHE INTERNAL "midi-smith flash map")
set(MIDISMITH_FLASH_BANK_ONE_ADDRESS 0x08000000 CACHE INTERNAL "midi-smith flash map")

set(MIDISMITH_BOOTLOADER_ADDRESS 0x08000000 CACHE INTERNAL "midi-smith flash map")
set(MIDISMITH_BOOTLOADER_SIZE_BYTES 131072 CACHE INTERNAL "midi-smith flash map")

set(MIDISMITH_STAGING_ADDRESS 0x08020000 CACHE INTERNAL "midi-smith flash map")
set(MIDISMITH_STAGING_SIZE_BYTES 393216 CACHE INTERNAL "midi-smith flash map")

set(MIDISMITH_BOOT_JOURNAL_ADDRESS 0x080C0000 CACHE INTERNAL "midi-smith flash map")
set(MIDISMITH_BOOT_JOURNAL_SIZE_BYTES 131072 CACHE INTERNAL "midi-smith flash map")

set(MIDISMITH_APPLICATION_CONFIG_ADDRESS 0x080E0000 CACHE INTERNAL "midi-smith flash map")
set(MIDISMITH_APPLICATION_CONFIG_SIZE_BYTES 131072 CACHE INTERNAL "midi-smith flash map")

set(MIDISMITH_APPLICATION_LOAD_ADDRESS 0x08100000 CACHE INTERNAL "midi-smith flash map")
set(MIDISMITH_APPLICATION_SLOT_SIZE_BYTES 393216 CACHE INTERNAL "midi-smith flash map")

# main.c relocates the vector table before HAL_Init inside its USER CODE zone, and C cannot include
# the generated C++ header.
function(midismith_firmware_load_address TARGET)
    target_compile_definitions(${TARGET} PRIVATE
        MIDISMITH_APPLICATION_LOAD_ADDRESS=${MIDISMITH_APPLICATION_LOAD_ADDRESS}
    )
endfunction()

# Reconciles the linker script with the map declared above, by reading the emitted .map.
function(midismith_check_flash_layout TARGET)
    cmake_parse_arguments(_MSCHECK "WITH_CONFIG_REGION" "" "" ${ARGN})

    set(CONFIG_ARGUMENTS "")
    if(_MSCHECK_WITH_CONFIG_REGION)
        set(CONFIG_ARGUMENTS
            -DEXPECTED_CONFIG_ORIGIN=${MIDISMITH_APPLICATION_CONFIG_ADDRESS}
            -DEXPECTED_CONFIG_SIZE=${MIDISMITH_APPLICATION_CONFIG_SIZE_BYTES}
        )
    endif()

    add_custom_command(TARGET ${TARGET} POST_BUILD
        COMMAND ${CMAKE_COMMAND}
                -DMAP_FILE=${CMAKE_BINARY_DIR}/${TARGET}.map
                -DEXPECTED_APPLICATION_ORIGIN=${MIDISMITH_APPLICATION_LOAD_ADDRESS}
                -DEXPECTED_APPLICATION_SIZE=${MIDISMITH_APPLICATION_SLOT_SIZE_BYTES}
                ${CONFIG_ARGUMENTS}
                -P ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/midismith_check_flash_layout.cmake
        COMMENT "Validating the flash layout of ${TARGET}"
        VERBATIM
    )
endfunction()

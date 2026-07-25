include_guard(GLOBAL)

# The flash map both STM32H743 firmware packages share. They carry the same part and will be
# installed by the same bootloader, so the map is a monorepo level fact rather than a per-package
# one. This file is the single source of truth: the linker scripts are checked against it at
# build time, and the C++ constants are generated from it.
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

set(MIDISMITH_FLASH_SECTOR_SIZE_BYTES 131072)

set(MIDISMITH_BOOTLOADER_ADDRESS 0x08000000)
set(MIDISMITH_BOOTLOADER_SIZE_BYTES 131072)

set(MIDISMITH_STAGING_ADDRESS 0x08020000)
set(MIDISMITH_STAGING_SIZE_BYTES 393216)

set(MIDISMITH_BOOT_JOURNAL_ADDRESS 0x080C0000)
set(MIDISMITH_BOOT_JOURNAL_SIZE_BYTES 131072)

set(MIDISMITH_APPLICATION_CONFIG_ADDRESS 0x080E0000)
set(MIDISMITH_APPLICATION_CONFIG_SIZE_BYTES 131072)
set(MIDISMITH_APPLICATION_CONFIG_BANK 1)
set(MIDISMITH_APPLICATION_CONFIG_SECTOR 7)

set(MIDISMITH_APPLICATION_LOAD_ADDRESS 0x08100000)
set(MIDISMITH_APPLICATION_SLOT_SIZE_BYTES 393216)

function(midismith_flash_layout_header TARGET)
    set(GENERATED_DIR "${CMAKE_BINARY_DIR}/generated")
    set(GENERATED_HEADER "${GENERATED_DIR}/flash-layout/flash_layout.hpp")
    set(GENERATOR_SCRIPT "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/generate_flash_layout_header.cmake")

    add_custom_command(
        OUTPUT "${GENERATED_HEADER}"
        COMMAND "${CMAKE_COMMAND}"
                -DOUTPUT_HEADER=${GENERATED_HEADER}
                -DFLASH_SECTOR_SIZE_BYTES=${MIDISMITH_FLASH_SECTOR_SIZE_BYTES}
                -DBOOTLOADER_ADDRESS=${MIDISMITH_BOOTLOADER_ADDRESS}
                -DBOOTLOADER_SIZE_BYTES=${MIDISMITH_BOOTLOADER_SIZE_BYTES}
                -DSTAGING_ADDRESS=${MIDISMITH_STAGING_ADDRESS}
                -DSTAGING_SIZE_BYTES=${MIDISMITH_STAGING_SIZE_BYTES}
                -DBOOT_JOURNAL_ADDRESS=${MIDISMITH_BOOT_JOURNAL_ADDRESS}
                -DBOOT_JOURNAL_SIZE_BYTES=${MIDISMITH_BOOT_JOURNAL_SIZE_BYTES}
                -DAPPLICATION_CONFIG_ADDRESS=${MIDISMITH_APPLICATION_CONFIG_ADDRESS}
                -DAPPLICATION_CONFIG_SIZE_BYTES=${MIDISMITH_APPLICATION_CONFIG_SIZE_BYTES}
                -DAPPLICATION_CONFIG_BANK=${MIDISMITH_APPLICATION_CONFIG_BANK}
                -DAPPLICATION_CONFIG_SECTOR=${MIDISMITH_APPLICATION_CONFIG_SECTOR}
                -DAPPLICATION_LOAD_ADDRESS=${MIDISMITH_APPLICATION_LOAD_ADDRESS}
                -DAPPLICATION_SLOT_SIZE_BYTES=${MIDISMITH_APPLICATION_SLOT_SIZE_BYTES}
                -P ${GENERATOR_SCRIPT}
        DEPENDS ${GENERATOR_SCRIPT} ${CMAKE_CURRENT_FUNCTION_LIST_FILE}
        COMMENT "Generating flash_layout.hpp"
        VERBATIM
    )

    add_custom_target(${TARGET}_flash_layout DEPENDS "${GENERATED_HEADER}")
    add_dependencies(${TARGET} ${TARGET}_flash_layout)
    target_include_directories(${TARGET} PRIVATE ${GENERATED_DIR}/flash-layout)

    # main.c relocates the vector table before HAL_Init inside its USER CODE zone, and C cannot
    # include the generated C++ header.
    target_compile_definitions(${TARGET} PRIVATE
        MIDISMITH_APPLICATION_LOAD_ADDRESS=${MIDISMITH_APPLICATION_LOAD_ADDRESS}
    )
endfunction()

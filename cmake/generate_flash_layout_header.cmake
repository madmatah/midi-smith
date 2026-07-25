cmake_minimum_required(VERSION 3.22)

foreach(required_variable
        OUTPUT_HEADER FLASH_SECTOR_SIZE_BYTES
        BOOTLOADER_ADDRESS BOOTLOADER_SIZE_BYTES
        STAGING_ADDRESS STAGING_SIZE_BYTES
        BOOT_JOURNAL_ADDRESS BOOT_JOURNAL_SIZE_BYTES
        APPLICATION_CONFIG_ADDRESS APPLICATION_CONFIG_SIZE_BYTES
        APPLICATION_CONFIG_BANK APPLICATION_CONFIG_SECTOR
        APPLICATION_LOAD_ADDRESS APPLICATION_SLOT_SIZE_BYTES)
    if(NOT DEFINED ${required_variable})
        message(FATAL_ERROR "${required_variable} must be provided to generate_flash_layout_header")
    endif()
endforeach()

set(header_contents
"#pragma once

#include <cstddef>
#include <cstdint>

namespace midismith::flash_layout {

inline constexpr std::size_t kFlashSectorSizeBytes = ${FLASH_SECTOR_SIZE_BYTES};

inline constexpr std::uint32_t kBootloaderAddress = ${BOOTLOADER_ADDRESS};
inline constexpr std::size_t kBootloaderSizeBytes = ${BOOTLOADER_SIZE_BYTES};

inline constexpr std::uint32_t kStagingAddress = ${STAGING_ADDRESS};
inline constexpr std::size_t kStagingSizeBytes = ${STAGING_SIZE_BYTES};

inline constexpr std::uint32_t kBootJournalAddress = ${BOOT_JOURNAL_ADDRESS};
inline constexpr std::size_t kBootJournalSizeBytes = ${BOOT_JOURNAL_SIZE_BYTES};

inline constexpr std::uint32_t kApplicationConfigAddress = ${APPLICATION_CONFIG_ADDRESS};
inline constexpr std::size_t kApplicationConfigSizeBytes = ${APPLICATION_CONFIG_SIZE_BYTES};
inline constexpr std::uint32_t kApplicationConfigBank = ${APPLICATION_CONFIG_BANK};
inline constexpr std::uint32_t kApplicationConfigSector = ${APPLICATION_CONFIG_SECTOR};

inline constexpr std::uint32_t kApplicationLoadAddress = ${APPLICATION_LOAD_ADDRESS};
inline constexpr std::size_t kApplicationSlotSizeBytes = ${APPLICATION_SLOT_SIZE_BYTES};

inline constexpr std::uint32_t kBankSizeBytes = 0x00100000;
inline constexpr std::uint32_t kBankOneAddress = 0x08000000;
inline constexpr std::uint32_t kBankTwoAddress = kBankOneAddress + kBankSizeBytes;

static_assert(kStagingSizeBytes >= kApplicationSlotSizeBytes,
              \"the staging slot must hold any image the application slot can take\");
static_assert(kApplicationLoadAddress >= kBankTwoAddress,
              \"the application executes from bank 2 so that every region it writes is in bank 1\");
static_assert(kStagingAddress < kBankTwoAddress && kBootJournalAddress < kBankTwoAddress &&
                  kApplicationConfigAddress < kBankTwoAddress,
              \"an erase on the bank the application executes from stalls the CPU for seconds\");
static_assert(kApplicationLoadAddress + kApplicationSlotSizeBytes <= kBankTwoAddress + kBankSizeBytes,
              \"the application slot must fit inside bank 2\");
static_assert(kBootloaderAddress == kBankOneAddress,
              \"the reset vector is fetched from the start of bank 1\");

}  // namespace midismith::flash_layout
")

get_filename_component(output_dir "${OUTPUT_HEADER}" DIRECTORY)
file(MAKE_DIRECTORY "${output_dir}")

set(tmp_header "${OUTPUT_HEADER}.tmp")
file(WRITE "${tmp_header}" "${header_contents}")
execute_process(COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${tmp_header}" "${OUTPUT_HEADER}")
file(REMOVE "${tmp_header}")

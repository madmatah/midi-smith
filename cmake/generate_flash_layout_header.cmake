cmake_minimum_required(VERSION 3.22)

# A missing -D leaves the variable defined but empty, which would emit a header that fails to
# compile instead of a diagnostic naming the variable.
foreach(required_variable
        OUTPUT_HEADER FLASH_SECTOR_SIZE_BYTES FLASH_BANK_SIZE_BYTES FLASH_BANK_ONE_ADDRESS
        BOOTLOADER_ADDRESS BOOTLOADER_SIZE_BYTES
        STAGING_ADDRESS STAGING_SIZE_BYTES
        BOOT_JOURNAL_ADDRESS BOOT_JOURNAL_SIZE_BYTES
        APPLICATION_CONFIG_ADDRESS APPLICATION_CONFIG_SIZE_BYTES
        APPLICATION_LOAD_ADDRESS APPLICATION_SLOT_SIZE_BYTES)
    if("${${required_variable}}" STREQUAL "")
        message(FATAL_ERROR
                "${required_variable} must be provided to generate_flash_layout_header, non-empty")
    endif()
endforeach()

set(header_contents
"#pragma once

#include <cstddef>
#include <cstdint>

namespace midismith::flash_layout {

inline constexpr std::uint32_t kFlashSectorSizeBytes = ${FLASH_SECTOR_SIZE_BYTES};
inline constexpr std::uint32_t kBankSizeBytes = ${FLASH_BANK_SIZE_BYTES};
inline constexpr std::uint32_t kBankOneAddress = ${FLASH_BANK_ONE_ADDRESS};
inline constexpr std::uint32_t kBankTwoAddress = kBankOneAddress + kBankSizeBytes;

inline constexpr std::uint32_t kBootloaderAddress = ${BOOTLOADER_ADDRESS};
inline constexpr std::uint32_t kBootloaderSizeBytes = ${BOOTLOADER_SIZE_BYTES};

inline constexpr std::uint32_t kStagingAddress = ${STAGING_ADDRESS};
inline constexpr std::uint32_t kStagingSizeBytes = ${STAGING_SIZE_BYTES};

inline constexpr std::uint32_t kBootJournalAddress = ${BOOT_JOURNAL_ADDRESS};
inline constexpr std::uint32_t kBootJournalSizeBytes = ${BOOT_JOURNAL_SIZE_BYTES};

inline constexpr std::uint32_t kApplicationConfigAddress = ${APPLICATION_CONFIG_ADDRESS};
inline constexpr std::uint32_t kApplicationConfigSizeBytes = ${APPLICATION_CONFIG_SIZE_BYTES};

inline constexpr std::uint32_t kApplicationLoadAddress = ${APPLICATION_LOAD_ADDRESS};
inline constexpr std::uint32_t kApplicationSlotSizeBytes = ${APPLICATION_SLOT_SIZE_BYTES};

[[nodiscard]] constexpr std::uint32_t BankOf(std::uint32_t address) noexcept {
  return address < kBankTwoAddress ? 1u : 2u;
}

[[nodiscard]] constexpr std::uint32_t SectorOf(std::uint32_t address) noexcept {
  return ((address - kBankOneAddress) % kBankSizeBytes) / kFlashSectorSizeBytes;
}

[[nodiscard]] constexpr bool IsSectorAligned(std::uint32_t address) noexcept {
  return ((address - kBankOneAddress) % kFlashSectorSizeBytes) == 0u;
}

inline constexpr std::uint32_t kApplicationConfigBank = BankOf(kApplicationConfigAddress);
inline constexpr std::uint32_t kApplicationConfigSector = SectorOf(kApplicationConfigAddress);

static_assert(IsSectorAligned(kBootloaderAddress) && IsSectorAligned(kStagingAddress) &&
                  IsSectorAligned(kBootJournalAddress) &&
                  IsSectorAligned(kApplicationConfigAddress) &&
                  IsSectorAligned(kApplicationLoadAddress),
              \"a region that starts mid sector cannot be erased without destroying its neighbour\");

static_assert(kStagingSizeBytes >= kApplicationSlotSizeBytes,
              \"the staging slot must hold any image the application slot can take\");

static_assert(BankOf(kApplicationLoadAddress) == 2,
              \"the application executes from bank 2 so that every region it writes is in bank 1\");

static_assert(BankOf(kStagingAddress) == 1 && BankOf(kBootJournalAddress) == 1 &&
                  BankOf(kApplicationConfigAddress) == 1,
              \"an erase on the bank the application executes from stalls the CPU for seconds\");

static_assert(kApplicationLoadAddress + kApplicationSlotSizeBytes <=
                  kBankTwoAddress + kBankSizeBytes,
              \"an application slot overrunning bank 2 would be erased into the rollback image\");

static_assert(kBootloaderAddress == kBankOneAddress,
              \"the reset vector is fetched from the start of bank 1\");

static_assert(kBootloaderAddress + kBootloaderSizeBytes <= kStagingAddress &&
                  kStagingAddress + kStagingSizeBytes <= kBootJournalAddress &&
                  kBootJournalAddress + kBootJournalSizeBytes <= kApplicationConfigAddress &&
                  kApplicationConfigAddress + kApplicationConfigSizeBytes <= kBankTwoAddress,
              \"two bank 1 regions that overlap would erase each other\");

}  // namespace midismith::flash_layout
")

get_filename_component(output_dir "${OUTPUT_HEADER}" DIRECTORY)
file(MAKE_DIRECTORY "${output_dir}")

set(tmp_header "${OUTPUT_HEADER}.tmp")
file(WRITE "${tmp_header}" "${header_contents}")
execute_process(COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${tmp_header}" "${OUTPUT_HEADER}")
file(REMOVE "${tmp_header}")

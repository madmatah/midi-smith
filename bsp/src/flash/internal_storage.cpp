#include "bsp/flash/internal_storage.hpp"

#include <cstdint>

#include "stm32h7xx_hal.h"

extern "C" {
extern std::uint32_t __flash_config_start__;
}

namespace bsp::flash {

namespace {
constexpr std::uintptr_t kConfigBaseAddress = 0x081E0000u;
constexpr std::uint32_t kConfigSectorNumber = 7;
constexpr std::size_t kSectorSizeBytes = 128 * 1024;

bool HasExpectedBaseAddress(const void* base_address) noexcept {
  return reinterpret_cast<std::uintptr_t>(base_address) == kConfigBaseAddress;
}

bool HasValidProgramParameters(std::size_t offset_bytes, const std::uint8_t* data,
                               std::size_t length_bytes) noexcept {
  if (data == nullptr) {
    return false;
  }
  if (length_bytes == 0 || (length_bytes % InternalStorage::kFlashWordSizeBytes) != 0) {
    return false;
  }
  if ((offset_bytes % InternalStorage::kFlashWordSizeBytes) != 0) {
    return false;
  }
  if (offset_bytes > kSectorSizeBytes) {
    return false;
  }
  if (length_bytes > (kSectorSizeBytes - offset_bytes)) {
    return false;
  }
  return true;
}

void ClearBank2ErrorFlags() noexcept {
  __HAL_FLASH_CLEAR_FLAG_BANK2(FLASH_FLAG_ALL_ERRORS_BANK2);
}
}  // namespace

const void* InternalStorage::BaseAddress() const noexcept {
  return &__flash_config_start__;
}

std::size_t InternalStorage::SectorSizeBytes() const noexcept {
  return kSectorSizeBytes;
}

OperationResult InternalStorage::EraseSector() noexcept {
  const void* base_address = BaseAddress();
  if (!HasExpectedBaseAddress(base_address)) {
    return OperationResult::kError;
  }

  HAL_FLASH_Unlock();
  ClearBank2ErrorFlags();

  FLASH_EraseInitTypeDef erase_config{};
  erase_config.TypeErase = FLASH_TYPEERASE_SECTORS;
  erase_config.Banks = FLASH_BANK_2;
  erase_config.Sector = kConfigSectorNumber;
  erase_config.NbSectors = 1;
  erase_config.VoltageRange = FLASH_VOLTAGE_RANGE_3;

  std::uint32_t sector_error = 0;
  HAL_StatusTypeDef status = HAL_FLASHEx_Erase(&erase_config, &sector_error);

  HAL_FLASH_Lock();

  if (status != HAL_OK || sector_error != 0xFFFFFFFFu) {
    return OperationResult::kError;
  }

  return OperationResult::kSuccess;
}

OperationResult InternalStorage::ProgramFlashWords(std::size_t offset_bytes,
                                                   const std::uint8_t* data,
                                                   std::size_t length_bytes) noexcept {
  const void* base_address_ptr = BaseAddress();
  if (!HasExpectedBaseAddress(base_address_ptr)) {
    return OperationResult::kError;
  }
  if (!HasValidProgramParameters(offset_bytes, data, length_bytes)) {
    return OperationResult::kError;
  }

  const auto base_address = reinterpret_cast<std::uintptr_t>(base_address_ptr);

  HAL_FLASH_Unlock();
  ClearBank2ErrorFlags();

  OperationResult result = OperationResult::kSuccess;
  std::size_t bytes_written = 0;

  while (bytes_written < length_bytes) {
    const auto destination_address =
        static_cast<std::uint32_t>(base_address + offset_bytes + bytes_written);
    const auto source_address =
        static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(data + bytes_written));

    HAL_StatusTypeDef status =
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, destination_address, source_address);

    if (status != HAL_OK) {
      result = OperationResult::kError;
      break;
    }

    bytes_written += kFlashWordSizeBytes;
  }

  HAL_FLASH_Lock();

  return result;
}

}  // namespace bsp::flash

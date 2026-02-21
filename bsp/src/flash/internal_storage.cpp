#include "bsp/flash/internal_storage.hpp"

#include <cstring>

#include "stm32h7xx_hal.h"

extern "C" {
extern std::uint32_t __flash_config_start__;
}

namespace bsp::flash {

namespace {
constexpr std::uint32_t kConfigSectorNumber = 7;
constexpr std::size_t kSectorSizeBytes = 128 * 1024;
}  // namespace

const void* InternalStorage::BaseAddress() const noexcept {
  return &__flash_config_start__;
}

std::size_t InternalStorage::SectorSizeBytes() const noexcept {
  return kSectorSizeBytes;
}

OperationResult InternalStorage::EraseSector() noexcept {
  HAL_FLASH_Unlock();

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

  auto base = reinterpret_cast<std::uint32_t>(BaseAddress());
  SCB_InvalidateDCache_by_Addr(reinterpret_cast<void*>(base),
                               static_cast<int32_t>(kSectorSizeBytes));

  return OperationResult::kSuccess;
}

OperationResult InternalStorage::ProgramFlashWords(std::size_t offset_bytes,
                                                   const std::uint8_t* data,
                                                   std::size_t length_bytes) noexcept {
  if (length_bytes == 0 || (length_bytes % kFlashWordSizeBytes) != 0) {
    return OperationResult::kError;
  }
  if ((offset_bytes % kFlashWordSizeBytes) != 0) {
    return OperationResult::kError;
  }

  auto base_address = reinterpret_cast<std::uint32_t>(BaseAddress());

  HAL_FLASH_Unlock();

  OperationResult result = OperationResult::kSuccess;
  std::size_t bytes_written = 0;

  while (bytes_written < length_bytes) {
    std::uint32_t destination_address = base_address + offset_bytes + bytes_written;
    std::uint32_t source_address = reinterpret_cast<std::uint32_t>(data + bytes_written);

    HAL_StatusTypeDef status =
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, destination_address, source_address);

    if (status != HAL_OK) {
      result = OperationResult::kError;
      break;
    }

    bytes_written += kFlashWordSizeBytes;
  }

  HAL_FLASH_Lock();

  SCB_InvalidateDCache_by_Addr(reinterpret_cast<void*>(base_address + offset_bytes),
                               static_cast<int32_t>(length_bytes));

  return result;
}

}  // namespace bsp::flash

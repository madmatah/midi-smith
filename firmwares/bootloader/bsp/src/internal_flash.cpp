#include "bsp/internal_flash.hpp"

#include "flash-layout/flash_layout.hpp"
#include "stm32h7xx_hal.h"

namespace midismith::bootloader::bsp {

namespace {

constexpr std::uint32_t kNoSectorInError = 0xFFFFFFFFu;

std::uint32_t HalBankOf(std::uint32_t address) noexcept {
  return midismith::flash_layout::BankOf(address) == 1 ? FLASH_BANK_1 : FLASH_BANK_2;
}

void ClearErrorFlagsOf(std::uint32_t address) noexcept {
  if (midismith::flash_layout::BankOf(address) == 1) {
    __HAL_FLASH_CLEAR_FLAG_BANK1(FLASH_FLAG_ALL_ERRORS_BANK1);
  } else {
    __HAL_FLASH_CLEAR_FLAG_BANK2(FLASH_FLAG_ALL_ERRORS_BANK2);
  }
}

}  // namespace

std::span<const std::uint8_t> InternalFlash::ReadRegion(std::uint32_t address,
                                                        std::size_t length_bytes) noexcept {
  return {reinterpret_cast<const std::uint8_t*>(address), length_bytes};
}

bool InternalFlash::EraseRegion(std::uint32_t address, std::size_t length_bytes) noexcept {
  if (length_bytes == 0 || !midismith::flash_layout::IsSectorAligned(address)) {
    return false;
  }

  const std::uint32_t sector_size = midismith::flash_layout::kFlashSectorSizeBytes;
  const auto sector_count =
      static_cast<std::uint32_t>((length_bytes + sector_size - 1) / sector_size);

  HAL_FLASH_Unlock();
  ClearErrorFlagsOf(address);

  FLASH_EraseInitTypeDef erase_config{};
  erase_config.TypeErase = FLASH_TYPEERASE_SECTORS;
  erase_config.Banks = HalBankOf(address);
  erase_config.Sector = midismith::flash_layout::SectorOf(address);
  erase_config.NbSectors = sector_count;
  erase_config.VoltageRange = FLASH_VOLTAGE_RANGE_3;

  std::uint32_t sector_in_error = 0;
  const HAL_StatusTypeDef status = HAL_FLASHEx_Erase(&erase_config, &sector_in_error);

  HAL_FLASH_Lock();

  return status == HAL_OK && sector_in_error == kNoSectorInError;
}

bool InternalFlash::ProgramRegion(std::uint32_t address,
                                  std::span<const std::uint8_t> data) noexcept {
  if (data.empty() || (data.size() % kFlashWordSizeBytes) != 0 ||
      (address % kFlashWordSizeBytes) != 0) {
    return false;
  }

  HAL_FLASH_Unlock();
  ClearErrorFlagsOf(address);

  bool programmed = true;
  for (std::size_t written_bytes = 0; written_bytes < data.size();
       written_bytes += kFlashWordSizeBytes) {
    const auto destination = static_cast<std::uint32_t>(address + written_bytes);
    const auto source =
        reinterpret_cast<std::uint32_t>(static_cast<const void*>(data.data() + written_bytes));

    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, destination, source) != HAL_OK) {
      programmed = false;
      break;
    }
  }

  HAL_FLASH_Lock();

  return programmed;
}

}  // namespace midismith::bootloader::bsp

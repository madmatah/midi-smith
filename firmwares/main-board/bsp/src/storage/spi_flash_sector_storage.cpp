#include "bsp/storage/spi_flash_sector_storage.hpp"

#include "bsp/stm32_spi_flash.hpp"

namespace midismith::main_board::bsp::storage {

using StorageOperationResult = midismith::bsp::storage::StorageOperationResult;

SpiFlashSectorStorage::SpiFlashSectorStorage(midismith::main_board::bsp::Stm32SpiFlash& flash,
                                             std::size_t sector_index) noexcept
    : flash_(flash),
      sector_base_address_(static_cast<std::uint32_t>(sector_index * kSectorSizeBytes)) {}

std::size_t SpiFlashSectorStorage::SectorSizeBytes() const noexcept {
  return kSectorSizeBytes;
}

StorageOperationResult SpiFlashSectorStorage::Read(std::size_t offset_bytes, std::uint8_t* buffer,
                                                   std::size_t length_bytes) const noexcept {
  if (offset_bytes + length_bytes > kSectorSizeBytes) {
    return StorageOperationResult::kError;
  }

  bool success = flash_.Read(sector_base_address_ + static_cast<std::uint32_t>(offset_bytes),
                             buffer, length_bytes);
  return success ? StorageOperationResult::kSuccess : StorageOperationResult::kError;
}

StorageOperationResult SpiFlashSectorStorage::EraseSector() noexcept {
  bool success = flash_.EraseSector(sector_base_address_);
  return success ? StorageOperationResult::kSuccess : StorageOperationResult::kError;
}

StorageOperationResult SpiFlashSectorStorage::Write(std::size_t offset_bytes,
                                                    const std::uint8_t* data,
                                                    std::size_t length_bytes) noexcept {
  if (offset_bytes + length_bytes > kSectorSizeBytes) {
    return StorageOperationResult::kError;
  }

  bool success = flash_.Write(sector_base_address_ + static_cast<std::uint32_t>(offset_bytes), data,
                              length_bytes);
  return success ? StorageOperationResult::kSuccess : StorageOperationResult::kError;
}

}  // namespace midismith::main_board::bsp::storage

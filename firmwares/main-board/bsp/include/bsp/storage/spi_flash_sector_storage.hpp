#pragma once

#include <cstddef>

#include "bsp-types/storage/flash_sector_storage_requirements.hpp"

namespace midismith::main_board::bsp {
class Stm32SpiFlash;
}

namespace midismith::main_board::bsp::storage {

class SpiFlashSectorStorage : public midismith::bsp::storage::FlashSectorStorageRequirements {
 public:
  static constexpr std::size_t kSectorSizeBytes = 4096;

  SpiFlashSectorStorage(midismith::main_board::bsp::Stm32SpiFlash& flash,
                        std::size_t sector_index) noexcept;

  std::size_t SectorSizeBytes() const noexcept override;
  midismith::bsp::storage::StorageOperationResult Read(
      std::size_t offset_bytes, std::uint8_t* buffer,
      std::size_t length_bytes) const noexcept override;
  midismith::bsp::storage::StorageOperationResult EraseSector() noexcept override;
  midismith::bsp::storage::StorageOperationResult Write(std::size_t offset_bytes,
                                                        const std::uint8_t* data,
                                                        std::size_t length_bytes) noexcept override;

 private:
  midismith::main_board::bsp::Stm32SpiFlash& flash_;
  std::uint32_t sector_base_address_;
};

}  // namespace midismith::main_board::bsp::storage

#pragma once

#include <cstddef>
#include <cstdint>

#include "bsp/storage/flash_sector_storage_requirements.hpp"

namespace midismith::adc_board::bsp::flash {

class InternalStorage : public midismith::bsp::storage::FlashSectorStorageRequirements {
 public:
  static constexpr std::size_t kFlashWordSizeBytes = 32;

  const void* BaseAddress() const noexcept override;
  std::size_t SectorSizeBytes() const noexcept override;
  midismith::bsp::storage::StorageOperationResult EraseSector() noexcept override;
  midismith::bsp::storage::StorageOperationResult ProgramFlashWords(
      std::size_t offset_bytes, const std::uint8_t* data,
      std::size_t length_bytes) noexcept override;
};

}  // namespace midismith::adc_board::bsp::flash

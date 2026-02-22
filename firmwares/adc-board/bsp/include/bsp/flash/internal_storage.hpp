#pragma once

#include <cstddef>
#include <cstdint>

#include "bsp/flash/storage_requirements.hpp"

namespace midismith::adc_board::bsp::flash {

class InternalStorage : public StorageRequirements {
 public:
  static constexpr std::size_t kFlashWordSizeBytes = 32;

  const void* BaseAddress() const noexcept override;
  std::size_t SectorSizeBytes() const noexcept override;
  OperationResult EraseSector() noexcept override;
  OperationResult ProgramFlashWords(std::size_t offset_bytes, const std::uint8_t* data,
                                    std::size_t length_bytes) noexcept override;
};

}  // namespace midismith::adc_board::bsp::flash

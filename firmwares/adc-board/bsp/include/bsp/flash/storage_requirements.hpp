#pragma once

#include <cstddef>
#include <cstdint>

namespace midismith::adc_board::bsp::flash {

enum class OperationResult { kSuccess, kError };

class StorageRequirements {
 public:
  virtual ~StorageRequirements() = default;
  virtual const void* BaseAddress() const noexcept = 0;
  virtual std::size_t SectorSizeBytes() const noexcept = 0;
  virtual OperationResult EraseSector() noexcept = 0;
  virtual OperationResult ProgramFlashWords(std::size_t offset_bytes, const std::uint8_t* data,
                                            std::size_t length_bytes) noexcept = 0;
};

}  // namespace midismith::adc_board::bsp::flash

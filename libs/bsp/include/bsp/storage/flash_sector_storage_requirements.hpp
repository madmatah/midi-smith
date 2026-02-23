#pragma once

#include <cstddef>
#include <cstdint>

#include "bsp/storage/storage_operation_result.hpp"

namespace midismith::bsp::storage {

class FlashSectorStorageRequirements {
 public:
  virtual ~FlashSectorStorageRequirements() = default;
  virtual const void* BaseAddress() const noexcept = 0;
  virtual std::size_t SectorSizeBytes() const noexcept = 0;
  virtual StorageOperationResult EraseSector() noexcept = 0;
  virtual StorageOperationResult ProgramFlashWords(std::size_t offset_bytes,
                                                   const std::uint8_t* data,
                                                   std::size_t length_bytes) noexcept = 0;
};

}  // namespace midismith::bsp::storage

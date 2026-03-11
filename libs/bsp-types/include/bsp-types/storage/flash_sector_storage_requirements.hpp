#pragma once

#include <cstddef>
#include <cstdint>

#include "bsp-types/storage/storage_operation_result.hpp"

namespace midismith::bsp::storage {

class FlashSectorStorageRequirements {
 public:
  virtual ~FlashSectorStorageRequirements() = default;
  virtual std::size_t SectorSizeBytes() const noexcept = 0;
  virtual StorageOperationResult Read(std::size_t offset_bytes, std::uint8_t* buffer,
                                      std::size_t length_bytes) const noexcept = 0;
  virtual StorageOperationResult EraseSector() noexcept = 0;
  virtual StorageOperationResult Write(std::size_t offset_bytes, const std::uint8_t* data,
                                       std::size_t length_bytes) noexcept = 0;
};

}  // namespace midismith::bsp::storage

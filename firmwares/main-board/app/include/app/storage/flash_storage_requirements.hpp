#pragma once

#include <cstddef>
#include <cstdint>

namespace midismith::main_board::app::storage {

class FlashStorageRequirements {
 public:
  virtual ~FlashStorageRequirements() = default;

  virtual bool Read(std::uint32_t address, void* buffer, std::size_t size_bytes) noexcept = 0;
  virtual bool Write(std::uint32_t address, const void* data, std::size_t size_bytes) noexcept = 0;
  virtual bool EraseSector(std::uint32_t address) noexcept = 0;
  virtual bool EraseChip() noexcept = 0;

  virtual std::size_t sector_size_bytes() const noexcept = 0;
  virtual std::size_t page_size_bytes() const noexcept = 0;
  virtual std::size_t chip_size_bytes() const noexcept = 0;
};

}  // namespace midismith::main_board::app::storage

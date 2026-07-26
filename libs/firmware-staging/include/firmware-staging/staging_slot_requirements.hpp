#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace midismith::firmware_staging {

class StagingSlotRequirements {
 public:
  virtual ~StagingSlotRequirements() = default;

  [[nodiscard]] virtual std::size_t CapacityBytes() const noexcept = 0;

  [[nodiscard]] virtual bool Erase() noexcept = 0;

  [[nodiscard]] virtual bool ProgramFlashWord(std::size_t offset_bytes,
                                              std::span<const std::uint8_t> word) noexcept = 0;

  [[nodiscard]] virtual std::span<const std::uint8_t> Contents() const noexcept = 0;
};

}  // namespace midismith::firmware_staging

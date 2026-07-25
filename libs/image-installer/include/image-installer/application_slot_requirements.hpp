#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace midismith::image_installer {

class ApplicationSlotRequirements {
 public:
  virtual ~ApplicationSlotRequirements() = default;

  [[nodiscard]] virtual std::span<const std::uint8_t> StagedContainer() const noexcept = 0;

  [[nodiscard]] virtual std::span<const std::uint8_t> ApplicationSlot() const noexcept = 0;

  [[nodiscard]] virtual bool EraseApplicationSlot(std::size_t length_bytes) noexcept = 0;

  [[nodiscard]] virtual bool ProgramApplicationSlot(
      std::span<const std::uint8_t> payload) noexcept = 0;
};

}  // namespace midismith::image_installer

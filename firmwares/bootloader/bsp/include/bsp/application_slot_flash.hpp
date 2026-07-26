#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include "firmware-installer/application_slot_requirements.hpp"

namespace midismith::bootloader::bsp {

class ApplicationSlotFlash final
    : public midismith::firmware_installer::ApplicationSlotRequirements {
 public:
  [[nodiscard]] std::span<const std::uint8_t> StagedContainer() const noexcept override;

  [[nodiscard]] std::span<const std::uint8_t> ApplicationSlot() const noexcept override;

  [[nodiscard]] bool EraseApplicationSlot(std::size_t length_bytes) noexcept override;

  [[nodiscard]] bool ProgramApplicationSlot(
      std::span<const std::uint8_t> payload) noexcept override;
};

}  // namespace midismith::bootloader::bsp

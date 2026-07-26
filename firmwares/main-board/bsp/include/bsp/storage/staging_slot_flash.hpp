#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include "firmware-staging/staging_slot_requirements.hpp"

namespace midismith::main_board::bsp::storage {

class StagingSlotFlash final : public midismith::firmware_staging::StagingSlotRequirements {
 public:
  [[nodiscard]] std::size_t CapacityBytes() const noexcept override;

  [[nodiscard]] bool Erase() noexcept override;

  [[nodiscard]] bool ProgramFlashWord(std::size_t offset_bytes,
                                      std::span<const std::uint8_t> word) noexcept override;

  [[nodiscard]] std::span<const std::uint8_t> Contents() const noexcept override;
};

}  // namespace midismith::main_board::bsp::storage

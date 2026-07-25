#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include "boot-control/boot_journal_storage_requirements.hpp"

namespace midismith::bootloader::bsp {

class JournalStorage final : public midismith::boot_control::BootJournalStorageRequirements {
 public:
  [[nodiscard]] std::span<const std::uint8_t> Sector() const noexcept override;

  [[nodiscard]] bool ProgramRecord(std::size_t offset_bytes,
                                   std::span<const std::uint8_t> record) noexcept override;

  [[nodiscard]] bool EraseSector() noexcept override;
};

}  // namespace midismith::bootloader::bsp

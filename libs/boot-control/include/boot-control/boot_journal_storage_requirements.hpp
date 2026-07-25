#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace midismith::boot_control {

class BootJournalStorageRequirements {
 public:
  virtual ~BootJournalStorageRequirements() = default;

  [[nodiscard]] virtual std::span<const std::uint8_t> Sector() const noexcept = 0;

  [[nodiscard]] virtual bool ProgramRecord(std::size_t offset_bytes,
                                           std::span<const std::uint8_t> record) noexcept = 0;

  [[nodiscard]] virtual bool EraseSector() noexcept = 0;
};

}  // namespace midismith::boot_control

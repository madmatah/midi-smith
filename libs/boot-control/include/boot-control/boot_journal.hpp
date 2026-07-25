#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

#include "boot-control/boot_journal_record.hpp"

namespace midismith::boot_control {

class AppendOnlyBootJournal {
 public:
  explicit AppendOnlyBootJournal(std::span<const std::uint8_t> sector) noexcept : sector_(sector) {}

  [[nodiscard]] std::size_t record_slot_count() const noexcept {
    return sector_.size() / kBootJournalRecordSizeBytes;
  }

  [[nodiscard]] std::optional<BootJournalRecord> LastValidRecord() const noexcept;

  [[nodiscard]] std::optional<std::size_t> FirstErasedSlotIndex() const noexcept;

  [[nodiscard]] std::optional<std::size_t> FirstErasedSlotOffsetBytes() const noexcept;

  [[nodiscard]] bool IsExhausted() const noexcept {
    return !FirstErasedSlotIndex().has_value();
  }

  [[nodiscard]] bool IsCoherent() const noexcept;

  [[nodiscard]] BootJournalRecord MakeSuccessorRecord(UpdateState state) const noexcept;

 private:
  [[nodiscard]] std::span<const std::uint8_t> SlotAt(std::size_t slot_index) const noexcept;

  std::span<const std::uint8_t> sector_;
};

}  // namespace midismith::boot_control

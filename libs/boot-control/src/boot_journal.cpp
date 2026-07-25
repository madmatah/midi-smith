#include "boot-control/boot_journal.hpp"

namespace midismith::boot_control {

std::span<const std::uint8_t> AppendOnlyBootJournal::SlotAt(std::size_t slot_index) const noexcept {
  return sector_.subspan(slot_index * kBootJournalRecordSizeBytes, kBootJournalRecordSizeBytes);
}

std::optional<BootJournalRecord> AppendOnlyBootJournal::LastValidRecord() const noexcept {
  std::optional<BootJournalRecord> newest_record;

  for (std::size_t slot_index = 0; slot_index < record_slot_count(); ++slot_index) {
    const std::span<const std::uint8_t> slot = SlotAt(slot_index);
    if (IsErasedRecordSlot(slot)) {
      break;
    }
    const auto record = BootJournalRecord::Deserialize(slot);
    if (record.has_value()) {
      newest_record = record;
    }
  }

  return newest_record;
}

std::optional<std::size_t> AppendOnlyBootJournal::FirstErasedSlotIndex() const noexcept {
  for (std::size_t slot_index = 0; slot_index < record_slot_count(); ++slot_index) {
    if (IsErasedRecordSlot(SlotAt(slot_index))) {
      return slot_index;
    }
  }

  return std::nullopt;
}

std::optional<std::size_t> AppendOnlyBootJournal::FirstErasedSlotOffsetBytes() const noexcept {
  const auto slot_index = FirstErasedSlotIndex();
  if (!slot_index.has_value()) {
    return std::nullopt;
  }

  return *slot_index * kBootJournalRecordSizeBytes;
}

bool AppendOnlyBootJournal::IsCoherent() const noexcept {
  const auto first_erased_slot_index = FirstErasedSlotIndex();
  if (!first_erased_slot_index.has_value()) {
    return true;
  }

  for (std::size_t slot_index = *first_erased_slot_index; slot_index < record_slot_count();
       ++slot_index) {
    if (!IsErasedRecordSlot(SlotAt(slot_index))) {
      return false;
    }
  }

  return true;
}

BootJournalRecord AppendOnlyBootJournal::MakeSuccessorRecord(UpdateState state) const noexcept {
  const auto previous_record = LastValidRecord();

  BootJournalRecord successor;
  if (previous_record.has_value()) {
    successor = *previous_record;
    successor.sequence_number = previous_record->sequence_number + 1;
  }
  successor.state = state;

  if (state == UpdateState::kIdle || state == UpdateState::kUpdateFailed) {
    successor.staged_payload_crc32 = 0;
    successor.staged_payload_size_bytes = 0;
    successor.staged_product_id = product_id::ProductId::kUnknown;
  }

  return successor;
}

}  // namespace midismith::boot_control

#include "boot-control/boot_journal_writer.hpp"

#include <array>

#include "boot-control/boot_journal.hpp"

namespace midismith::boot_control {

bool BootJournalWriter::Append(UpdateState state) noexcept {
  const AppendOnlyBootJournal journal{storage_.Sector()};
  return Append(journal.MakeSuccessorRecord(state));
}

bool BootJournalWriter::Append(const BootJournalRecord& record) noexcept {
  std::array<std::uint8_t, kBootJournalRecordSizeBytes> serialized_record{};
  if (!record.Serialize(serialized_record).has_value()) {
    return false;
  }

  {
    const AppendOnlyBootJournal journal{storage_.Sector()};
    if (journal.IsExhausted() || !journal.IsCoherent()) {
      if (!storage_.EraseSector()) {
        return false;
      }
    }
  }

  const AppendOnlyBootJournal journal{storage_.Sector()};
  const auto slot_offset_bytes = journal.FirstErasedSlotOffsetBytes();
  if (!slot_offset_bytes.has_value()) {
    return false;
  }

  return storage_.ProgramRecord(*slot_offset_bytes, serialized_record);
}

}  // namespace midismith::boot_control

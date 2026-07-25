#pragma once

#include "boot-control/boot_journal_record.hpp"
#include "boot-control/boot_journal_storage_requirements.hpp"

namespace midismith::boot_control {

class BootJournalWriter {
 public:
  explicit BootJournalWriter(BootJournalStorageRequirements& storage) noexcept
      : storage_(storage) {}

  [[nodiscard]] bool Append(UpdateState state) noexcept;

  [[nodiscard]] bool Append(const BootJournalRecord& record) noexcept;

 private:
  BootJournalStorageRequirements& storage_;
};

}  // namespace midismith::boot_control

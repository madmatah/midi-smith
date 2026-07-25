#pragma once

#include <cstdint>
#include <optional>

#include "boot-control/boot_journal_record.hpp"

namespace midismith::boot_control {

enum class BootAction : std::uint8_t {
  kInstallStagedImage = 0,
  kBootApplication,
  kMarkUpdateFailed,
  kWaitForRecovery,
};

struct BootInputs {
  std::optional<BootJournalRecord> last_record;
  bool staged_image_installable = false;
  bool application_slot_valid = false;
};

[[nodiscard]] BootAction DecideBootAction(const BootInputs& inputs) noexcept;

}  // namespace midismith::boot_control

#include "boot-control/boot_decision.hpp"

namespace midismith::boot_control {

namespace {

bool AnnouncesAnUpdate(const BootJournalRecord& record) noexcept {
  return record.state == UpdateState::kUpdatePending ||
         record.state == UpdateState::kUpdateInProgress;
}

}  // namespace

BootAction DecideBootAction(const BootInputs& inputs) noexcept {
  if (inputs.last_record.has_value() && AnnouncesAnUpdate(*inputs.last_record)) {
    if (inputs.staged_image_installable) {
      return BootAction::kInstallStagedImage;
    }
    return BootAction::kMarkUpdateFailed;
  }

  if (inputs.application_slot_valid) {
    return BootAction::kBootApplication;
  }

  return BootAction::kWaitForRecovery;
}

}  // namespace midismith::boot_control

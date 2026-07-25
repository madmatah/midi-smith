#if defined(UNIT_TESTS)

#include "boot-control/boot_decision.hpp"

#include <catch2/catch_test_macros.hpp>

#include "boot-control/boot_journal_record.hpp"

namespace {

using midismith::boot_control::BootAction;
using midismith::boot_control::BootInputs;
using midismith::boot_control::BootJournalRecord;
using midismith::boot_control::DecideBootAction;
using midismith::boot_control::UpdateState;

BootJournalRecord MakeRecord(UpdateState state) {
  BootJournalRecord record;
  record.state = state;
  return record;
}

}  // namespace

TEST_CASE("The DecideBootAction function") {
  SECTION("When an update is pending and its staged image is installable") {
    SECTION("Should install it, even though the application slot still holds a working image") {
      const BootInputs inputs{.last_record = MakeRecord(UpdateState::kUpdatePending),
                              .staged_image_installable = true,
                              .application_slot_valid = true};

      REQUIRE(DecideBootAction(inputs) == BootAction::kInstallStagedImage);
    }
  }

  SECTION("When power was cut during a previous install") {
    SECTION("Should install again, because a half copied slot is only repaired by redoing it") {
      const BootInputs inputs{.last_record = MakeRecord(UpdateState::kUpdateInProgress),
                              .staged_image_installable = true,
                              .application_slot_valid = false};

      REQUIRE(DecideBootAction(inputs) == BootAction::kInstallStagedImage);
    }
  }

  SECTION("When an update is pending but its staged image cannot be installed") {
    SECTION("Should record the failure rather than retry it forever at every power up") {
      const BootInputs inputs{.last_record = MakeRecord(UpdateState::kUpdatePending),
                              .staged_image_installable = false,
                              .application_slot_valid = true};

      REQUIRE(DecideBootAction(inputs) == BootAction::kMarkUpdateFailed);
    }
  }

  SECTION("When a previous update was already recorded as failed") {
    SECTION("Should boot the application that is still in place") {
      const BootInputs inputs{.last_record = MakeRecord(UpdateState::kUpdateFailed),
                              .staged_image_installable = true,
                              .application_slot_valid = true};

      REQUIRE(DecideBootAction(inputs) == BootAction::kBootApplication);
    }
  }

  SECTION("When the journal is idle and the application slot is valid") {
    SECTION("Should boot the application") {
      const BootInputs inputs{.last_record = MakeRecord(UpdateState::kIdle),
                              .staged_image_installable = false,
                              .application_slot_valid = true};

      REQUIRE(DecideBootAction(inputs) == BootAction::kBootApplication);
    }
  }

  SECTION("When the board has never been updated and holds no journal at all") {
    SECTION("Should boot the application, so a freshly flashed board starts normally") {
      const BootInputs inputs{.last_record = std::nullopt,
                              .staged_image_installable = false,
                              .application_slot_valid = true};

      REQUIRE(DecideBootAction(inputs) == BootAction::kBootApplication);
    }
  }

  SECTION("When the application slot holds no valid image and no update is pending") {
    SECTION("Should wait for recovery rather than jump into whatever is at the slot address") {
      const BootInputs inputs{.last_record = MakeRecord(UpdateState::kIdle),
                              .staged_image_installable = false,
                              .application_slot_valid = false};

      REQUIRE(DecideBootAction(inputs) == BootAction::kWaitForRecovery);
    }
  }

  SECTION("When a staged image is installable but no update was ever announced") {
    SECTION("Should boot the application, because leftover staging is not an instruction") {
      const BootInputs inputs{.last_record = MakeRecord(UpdateState::kIdle),
                              .staged_image_installable = true,
                              .application_slot_valid = true};

      REQUIRE(DecideBootAction(inputs) == BootAction::kBootApplication);
    }
  }

  SECTION("When nothing is bootable and the announced update cannot be installed") {
    SECTION("Should record the failure first, so the next boot reports a board needing recovery") {
      const BootInputs inputs{.last_record = MakeRecord(UpdateState::kUpdatePending),
                              .staged_image_installable = false,
                              .application_slot_valid = false};

      REQUIRE(DecideBootAction(inputs) == BootAction::kMarkUpdateFailed);

      const BootInputs after_recording{.last_record = MakeRecord(UpdateState::kUpdateFailed),
                                       .staged_image_installable = false,
                                       .application_slot_valid = false};

      REQUIRE(DecideBootAction(after_recording) == BootAction::kWaitForRecovery);
    }
  }
}

#endif

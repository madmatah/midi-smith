#include "app/application.hpp"

#include <cstdint>

#include "boot-control/boot_decision.hpp"
#include "boot-control/boot_journal.hpp"
#include "boot-control/boot_journal_record.hpp"
#include "boot-control/boot_journal_writer.hpp"
#include "bsp/application_launcher.hpp"
#include "bsp/application_slot_flash.hpp"
#include "bsp/journal_storage.hpp"
#include "bsp/status_led.hpp"
#include "firmware-image/image_installability.hpp"
#include "flash-layout/flash_layout.hpp"
#include "image-installer/staged_image_installer.hpp"
#include "product-id/product_id.hpp"

namespace midismith::bootloader::app {

namespace {

using boot_control::AppendOnlyBootJournal;
using boot_control::BootAction;
using boot_control::BootInputs;
using boot_control::BootJournalRecord;
using boot_control::BootJournalWriter;
using boot_control::DecideBootAction;
using boot_control::UpdateState;
using image_installer::InstallOutcome;
using image_installer::StagedImageDescription;
using image_installer::StagedImageInstaller;

constexpr std::uint32_t kUnbootableBlinkPeriodMs = 120;

firmware_image::TargetConstraints SlotConstraints() noexcept {
  return firmware_image::TargetConstraints{
      .expected_product_id = product_id::ProductId::kUnknown,
      .expected_load_address = flash_layout::kApplicationLoadAddress,
      .maximum_payload_size_bytes = flash_layout::kApplicationSlotSizeBytes,
      .supported_protocol_version = 0,
  };
}

StagedImageDescription AnnouncedBy(const BootJournalRecord& record) noexcept {
  return StagedImageDescription{record.staged_payload_crc32, record.staged_payload_size_bytes,
                                record.staged_product_id};
}

}  // namespace

void Application::Run() noexcept {
  bsp::JournalStorage journal_storage;
  bsp::ApplicationSlotFlash application_slot;
  BootJournalWriter journal_writer{journal_storage};
  StagedImageInstaller installer{application_slot, SlotConstraints()};

  while (true) {
    const AppendOnlyBootJournal journal{journal_storage.Sector()};
    const auto last_record = journal.LastValidRecord();

    BootInputs inputs;
    inputs.last_record = last_record;
    inputs.application_slot_valid = installer.ApplicationSlotHoldsBootableImage();
    inputs.staged_image_installable =
        last_record.has_value() && installer.AcceptsStagedImage(AnnouncedBy(*last_record));

    switch (DecideBootAction(inputs)) {
      case BootAction::kBootApplication:
        bsp::ApplicationLauncher::LaunchAt(flash_layout::kApplicationLoadAddress);
        break;

      case BootAction::kInstallStagedImage: {
        if (!journal_writer.Append(UpdateState::kUpdateInProgress)) {
          bsp::StatusLed::BlinkForever(kUnbootableBlinkPeriodMs);
        }
        bsp::StatusLed::TurnOn();
        const bool installed = installer.Install() == InstallOutcome::kInstalled;
        bsp::StatusLed::TurnOff();
        if (!journal_writer.Append(installed ? UpdateState::kIdle : UpdateState::kUpdateFailed)) {
          bsp::StatusLed::BlinkForever(kUnbootableBlinkPeriodMs);
        }
        break;
      }

      case BootAction::kMarkUpdateFailed:
        if (!journal_writer.Append(UpdateState::kUpdateFailed)) {
          bsp::StatusLed::BlinkForever(kUnbootableBlinkPeriodMs);
        }
        break;

      case BootAction::kWaitForRecovery:
        bsp::StatusLed::BlinkForever(kUnbootableBlinkPeriodMs);
    }
  }
}

}  // namespace midismith::bootloader::app

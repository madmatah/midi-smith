#include "app/application.hpp"

#include <cstdint>
#include <span>

#include "boot-control/boot_decision.hpp"
#include "boot-control/boot_journal.hpp"
#include "boot-control/boot_journal_record.hpp"
#include "boot-control/boot_journal_writer.hpp"
#include "bsp/application_launcher.hpp"
#include "bsp/internal_flash.hpp"
#include "bsp/journal_storage.hpp"
#include "bsp/status_led.hpp"
#include "checksum/crc32.hpp"
#include "firmware-image/image_header.hpp"
#include "firmware-image/image_installability.hpp"
#include "flash-layout/flash_layout.hpp"
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
using bsp::ApplicationLauncher;
using bsp::InternalFlash;
using bsp::JournalStorage;
using bsp::StatusLed;
using firmware_image::ContainerPayload;
using firmware_image::EvaluateImageInstallability;
using firmware_image::ImageInstallability;
using firmware_image::ParseImageHeader;
using firmware_image::TargetConstraints;

constexpr std::uint32_t kUnbootableBlinkPeriodMs = 120;
constexpr std::uint32_t kAddressRegionMask = 0xFF000000u;
constexpr std::uint32_t kDtcmRamRegion = 0x20000000u;
constexpr std::uint32_t kAxiSramRegion = 0x24000000u;

std::span<const std::uint8_t> StagedContainer() noexcept {
  return InternalFlash::ReadRegion(flash_layout::kStagingAddress, flash_layout::kStagingSizeBytes);
}

TargetConstraints ConstraintsForProductDeclaredByTheImage(
    product_id::ProductId declared_product_id) noexcept {
  return TargetConstraints{
      .expected_product_id = declared_product_id,
      .expected_load_address = flash_layout::kApplicationLoadAddress,
      .maximum_payload_size_bytes = flash_layout::kApplicationSlotSizeBytes,
      .supported_protocol_version = 0,
  };
}

bool IsStagedImageInstallable(const BootJournalRecord& announcement) noexcept {
  const auto parsed = ParseImageHeader(StagedContainer());
  if (!parsed.is_valid()) {
    return false;
  }

  if (parsed.header.payload_crc32 != announcement.staged_payload_crc32 ||
      parsed.header.payload_size_bytes != announcement.staged_payload_size_bytes ||
      parsed.header.product_id != announcement.staged_product_id) {
    return false;
  }

  const auto payload = ContainerPayload(parsed.header, StagedContainer());
  if (!payload.has_value()) {
    return false;
  }

  return EvaluateImageInstallability(
             parsed.header, *payload,
             ConstraintsForProductDeclaredByTheImage(parsed.header.product_id)) ==
         ImageInstallability::kInstallable;
}

bool ApplicationSlotHoldsBootableImage() noexcept {
  const auto* vector_table =
      reinterpret_cast<const std::uint32_t*>(flash_layout::kApplicationLoadAddress);
  const std::uint32_t stack_pointer = vector_table[0];
  const std::uint32_t reset_handler = vector_table[1];

  const std::uint32_t stack_region = stack_pointer & kAddressRegionMask;
  const bool stack_pointer_is_in_ram =
      stack_region == kDtcmRamRegion || stack_region == kAxiSramRegion;
  const bool reset_handler_is_in_slot = reset_handler >= flash_layout::kApplicationLoadAddress &&
                                        reset_handler < flash_layout::kApplicationLoadAddress +
                                                            flash_layout::kApplicationSlotSizeBytes;

  return stack_pointer_is_in_ram && reset_handler_is_in_slot;
}

bool CopyStagedImageIntoApplicationSlot() noexcept {
  const auto parsed = ParseImageHeader(StagedContainer());
  const auto payload = ContainerPayload(parsed.header, StagedContainer());
  if (!payload.has_value()) {
    return false;
  }

  if (!InternalFlash::EraseRegion(flash_layout::kApplicationLoadAddress, payload->size())) {
    return false;
  }
  if (!InternalFlash::ProgramRegion(flash_layout::kApplicationLoadAddress, *payload)) {
    return false;
  }

  const auto installed =
      InternalFlash::ReadRegion(flash_layout::kApplicationLoadAddress, payload->size());
  return checksum::ComputeCrc32(installed) == parsed.header.payload_crc32;
}

}  // namespace

void Application::Run() noexcept {
  JournalStorage journal_storage;
  BootJournalWriter journal_writer{journal_storage};

  while (true) {
    const AppendOnlyBootJournal journal{journal_storage.Sector()};
    const auto last_record = journal.LastValidRecord();

    BootInputs inputs;
    inputs.last_record = last_record;
    inputs.application_slot_valid = ApplicationSlotHoldsBootableImage();
    inputs.staged_image_installable =
        last_record.has_value() && IsStagedImageInstallable(*last_record);

    switch (DecideBootAction(inputs)) {
      case BootAction::kBootApplication:
        ApplicationLauncher::LaunchAt(flash_layout::kApplicationLoadAddress);
        break;

      case BootAction::kInstallStagedImage: {
        if (!journal_writer.Append(UpdateState::kUpdateInProgress)) {
          StatusLed::BlinkForever(kUnbootableBlinkPeriodMs);
        }
        StatusLed::TurnOn();
        const bool installed = CopyStagedImageIntoApplicationSlot();
        StatusLed::TurnOff();
        if (!journal_writer.Append(installed ? UpdateState::kIdle : UpdateState::kUpdateFailed)) {
          StatusLed::BlinkForever(kUnbootableBlinkPeriodMs);
        }
        break;
      }

      case BootAction::kMarkUpdateFailed:
        if (!journal_writer.Append(UpdateState::kUpdateFailed)) {
          StatusLed::BlinkForever(kUnbootableBlinkPeriodMs);
        }
        break;

      case BootAction::kWaitForRecovery:
        StatusLed::BlinkForever(kUnbootableBlinkPeriodMs);
    }
  }
}

}  // namespace midismith::bootloader::app

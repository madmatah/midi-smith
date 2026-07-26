#include "app/update/self_update_service.hpp"

#include <algorithm>
#include <span>

#include "boot-control/boot_journal_record.hpp"
#include "flash-layout/flash_layout.hpp"
#include "update-catalogue/update_catalogue.hpp"

namespace midismith::main_board::app::update {

namespace {

using midismith::firmware_staging::StagingOutcome;
using midismith::firmware_staging::StagingWriter;
using midismith::product_id::ProductId;
using midismith::update_catalogue::CatalogueEntry;
using midismith::update_catalogue::CatalogueStatus;
using midismith::update_catalogue::EvaluateUpdateNeed;
using midismith::update_catalogue::ImagePathFor;
using midismith::update_catalogue::UpdateCatalogue;
using midismith::update_catalogue::UpdateNeed;

midismith::firmware_image::TargetConstraints MakeSelfConstraints() noexcept {
  midismith::firmware_image::TargetConstraints constraints;
  constraints.expected_product_id = ProductId::kMainBoard;
  constraints.expected_load_address = midismith::flash_layout::kApplicationLoadAddress;
  constraints.maximum_payload_size_bytes = midismith::flash_layout::kApplicationSlotSizeBytes;
  constraints.supported_protocol_version = 0;
  return constraints;
}

}  // namespace

SelfUpdateService::SelfUpdateService(midismith::update_catalogue::ImageSourceRequirements& images,
                                     midismith::firmware_staging::StagingSlotRequirements& staging,
                                     midismith::boot_control::BootJournalWriter& journal,
                                     std::string_view installed_version) noexcept
    : images_(images),
      staging_(staging),
      journal_(journal),
      installed_version_(installed_version) {}

SelfUpdateOutcome SelfUpdateService::Run() noexcept {
  UpdateCatalogue catalogue{images_};
  const CatalogueEntry entry = catalogue.Lookup(ProductId::kMainBoard);

  if (entry.status == CatalogueStatus::kNoImageOnCard) {
    return SelfUpdateOutcome::kNoImageOnCard;
  }

  switch (EvaluateUpdateNeed(entry, installed_version_)) {
    case UpdateNeed::kUpToDate:
      return SelfUpdateOutcome::kAlreadyRunningThisBuild;
    case UpdateNeed::kNoImage:
      return SelfUpdateOutcome::kNoImageOnCard;
    case UpdateNeed::kImageUnusable:
      return SelfUpdateOutcome::kImageUnusable;
    case UpdateNeed::kUpdateAvailable:
    case UpdateNeed::kInstalledVersionUnknown:
      break;
  }

  StagingWriter writer{staging_};
  last_staging_outcome_ = writer.Begin(entry.container_size_bytes);
  if (last_staging_outcome_ != StagingOutcome::kStaged) {
    return SelfUpdateOutcome::kStagingFailed;
  }

  const std::string_view path = ImagePathFor(ProductId::kMainBoard);
  std::uint32_t copied_bytes = 0;
  while (copied_bytes < entry.container_size_bytes) {
    const std::size_t wanted =
        std::min<std::size_t>(copy_buffer_.size(), entry.container_size_bytes - copied_bytes);
    const auto read_bytes =
        images_.ReadAt(path, copied_bytes, std::span{copy_buffer_}.first(wanted));
    if (!read_bytes.has_value() || *read_bytes == 0) {
      last_staging_outcome_ = StagingOutcome::kFewerBytesThanAnnounced;
      return SelfUpdateOutcome::kStagingFailed;
    }

    last_staging_outcome_ = writer.Write(std::span{copy_buffer_}.first(*read_bytes));
    if (last_staging_outcome_ != StagingOutcome::kStaged) {
      return SelfUpdateOutcome::kStagingFailed;
    }
    copied_bytes += static_cast<std::uint32_t>(*read_bytes);
  }

  last_staging_outcome_ = writer.Finish(MakeSelfConstraints());
  if (last_staging_outcome_ != StagingOutcome::kStaged) {
    return SelfUpdateOutcome::kStagingFailed;
  }

  if (!journal_.Append(midismith::boot_control::UpdateState::kUpdatePending)) {
    return SelfUpdateOutcome::kJournalWriteFailed;
  }

  return SelfUpdateOutcome::kStagedAndPending;
}

}  // namespace midismith::main_board::app::update

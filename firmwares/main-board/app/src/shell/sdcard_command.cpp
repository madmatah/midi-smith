#include "app/shell/sdcard_command.hpp"

#include <array>
#include <cinttypes>
#include <cstdint>
#include <cstdio>

#include "update-catalogue/update_catalogue.hpp"

namespace midismith::main_board::app::shell {

namespace {

using midismith::product_id::ProductId;
using midismith::update_catalogue::CatalogueStatus;
using midismith::update_catalogue::EvaluateUpdateNeed;
using midismith::update_catalogue::ImagePathFor;
using midismith::update_catalogue::UpdateCatalogue;
using midismith::update_catalogue::UpdateNeed;

constexpr std::size_t kNumberTextCapacity = 24;

std::string_view NameOf(ProductId product) noexcept {
  switch (product) {
    case ProductId::kMainBoard:
      return "main-board";
    case ProductId::kAdcBoard:
      return "adc-board";
    case ProductId::kUnknown:
    default:
      return "unknown";
  }
}

std::string_view DescribeStatus(CatalogueStatus status) noexcept {
  switch (status) {
    case CatalogueStatus::kImageAvailable:
      return "present";
    case CatalogueStatus::kNoImageOnCard:
      return "absent";
    case CatalogueStatus::kUnreadable:
      return "unreadable";
    case CatalogueStatus::kNotAnImage:
      return "not a firmware image";
    case CatalogueStatus::kWrongProduct:
      return "targets another board";
  }
  return "unknown";
}

std::string_view DescribeMountResult(midismith::bsp::storage::VolumeMountResult result) noexcept {
  switch (result) {
    case midismith::bsp::storage::VolumeMountResult::kDriveNotReady:
      return "the drive reported itself not ready";
    case midismith::bsp::storage::VolumeMountResult::kNoFileSystem:
      return "no FAT file system was found, is the card FAT32?";
    case midismith::bsp::storage::VolumeMountResult::kVolumeLockTimedOut:
      return "the volume lock timed out, another task still holds it";
    case midismith::bsp::storage::VolumeMountResult::kFileSystemInternalError:
      return "the file system layer could not create its lock";
    case midismith::bsp::storage::VolumeMountResult::kMounted:
    case midismith::bsp::storage::VolumeMountResult::kNotAttempted:
    case midismith::bsp::storage::VolumeMountResult::kOtherFailure:
      break;
  }
  return "the mount failed";
}

std::string_view DescribeMountFailure(
    midismith::bsp::storage::SdCardBringUpOutcome outcome) noexcept {
  switch (outcome) {
    case midismith::bsp::storage::SdCardBringUpOutcome::kNoCardAnswered:
      return "no card answered, is one inserted?";
    case midismith::bsp::storage::SdCardBringUpOutcome::kWideBusRefused:
      return "the card refused the 4-bit bus";
    case midismith::bsp::storage::SdCardBringUpOutcome::kReady:
      return "the card answered but its file system could not be read, is it FAT32?";
    case midismith::bsp::storage::SdCardBringUpOutcome::kNeverAttempted:
      return "the file system layer refused before the driver ever reached the card";
  }
  return "could not mount the card";
}

std::string_view DescribeNeed(UpdateNeed need) noexcept {
  switch (need) {
    case UpdateNeed::kUpToDate:
      return "already running this build";
    case UpdateNeed::kUpdateAvailable:
      return "update available";
    case UpdateNeed::kNoImage:
      return "nothing offered";
    case UpdateNeed::kImageUnusable:
      return "unusable, will not be installed";
  }
  return "unknown";
}

void WriteUnsigned(midismith::io::WritableStreamRequirements& out, std::uint32_t value) noexcept {
  std::array<char, kNumberTextCapacity> text{};
  std::snprintf(text.data(), text.size(), "%" PRIu32, value);
  out.Write(text.data());
}

}  // namespace

SdCardCommand::SdCardCommand(RemovableStorageRequirements& storage,
                             midismith::update_catalogue::ImageSourceRequirements& images,
                             std::string_view installed_version) noexcept
    : storage_(storage), images_(images), installed_version_(installed_version) {}

void SdCardCommand::ReportImageFor(ProductId product,
                                   midismith::io::WritableStreamRequirements& out) noexcept {
  UpdateCatalogue catalogue{images_};
  const auto entry = catalogue.Lookup(product);

  out.Write("  ");
  out.Write(ImagePathFor(product));
  out.Write("  ");
  out.Write(DescribeStatus(entry.status));
  out.Write("\r\n");

  if (entry.status == CatalogueStatus::kNoImageOnCard) {
    return;
  }

  out.Write("      product    ");
  out.Write(NameOf(entry.header.product_id));
  out.Write("\r\n      version    ");
  out.Write(entry.header.version_string.data());
  out.Write("\r\n      built      ");
  out.Write(entry.header.build_date.data());
  out.Write("\r\n      payload    ");
  WriteUnsigned(out, entry.header.payload_size_bytes);
  out.Write(" bytes\r\n      container  ");
  WriteUnsigned(out, entry.container_size_bytes);
  out.Write(" bytes\r\n      verdict    ");

  const std::string_view running =
      product == ProductId::kMainBoard ? installed_version_ : std::string_view{};
  out.Write(DescribeNeed(EvaluateUpdateNeed(entry, running)));
  out.Write("\r\n");
}

void SdCardCommand::Run(int argc, char** argv,
                        midismith::io::WritableStreamRequirements& out) noexcept {
  (void) argv;
  if (argc != 1) {
    out.Write("usage: sdcard\r\n");
    return;
  }

  if (!storage_.Mount()) {
    out.Write("sdcard: ");
    out.Write(DescribeMountResult(storage_.last_mount_result()));
    out.Write("\r\n        card bring-up: ");
    out.Write(DescribeMountFailure(storage_.last_bring_up_outcome()));
    out.Write("\r\n");
    return;
  }

  out.Write("sdcard: mounted\r\n");
  ReportImageFor(ProductId::kMainBoard, out);
  ReportImageFor(ProductId::kAdcBoard, out);

  storage_.Unmount();
}

}  // namespace midismith::main_board::app::shell

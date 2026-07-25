#include "image-installer/staged_image_installer.hpp"

#include <optional>
#include <span>

#include "checksum/crc32.hpp"
#include "firmware-image/image_header.hpp"

namespace midismith::image_installer {

namespace {

using firmware_image::ContainerPayload;
using firmware_image::EvaluateImageInstallability;
using firmware_image::ImageHeader;
using firmware_image::ImageInstallability;
using firmware_image::ParseImageHeader;

constexpr std::uint32_t kAddressRegionMask = 0xFF000000u;
constexpr std::uint32_t kDtcmRamRegion = 0x20000000u;
constexpr std::uint32_t kAxiSramRegion = 0x24000000u;
constexpr std::size_t kStackPointerWordIndex = 0;
constexpr std::size_t kResetHandlerWordIndex = 1;
constexpr std::size_t kVectorTableProbeSizeBytes = 2 * sizeof(std::uint32_t);

struct AcceptedImage {
  ImageHeader header;
  std::span<const std::uint8_t> payload;
};

std::optional<AcceptedImage> AcceptStagedContainer(
    std::span<const std::uint8_t> container,
    const firmware_image::TargetConstraints& constraints) noexcept {
  const auto parsed = ParseImageHeader(container);
  if (!parsed.is_valid()) {
    return std::nullopt;
  }

  const auto payload = ContainerPayload(parsed.header, container);
  if (!payload.has_value()) {
    return std::nullopt;
  }

  firmware_image::TargetConstraints resolved = constraints;
  resolved.expected_product_id = parsed.header.product_id;

  if (EvaluateImageInstallability(parsed.header, *payload, resolved) !=
      ImageInstallability::kInstallable) {
    return std::nullopt;
  }

  return AcceptedImage{parsed.header, *payload};
}

std::uint32_t ReadWord(std::span<const std::uint8_t> region, std::size_t word_index) noexcept {
  std::uint32_t word = 0;
  for (std::size_t byte_index = 0; byte_index < sizeof(std::uint32_t); ++byte_index) {
    word |= static_cast<std::uint32_t>(region[word_index * sizeof(std::uint32_t) + byte_index])
            << (8u * byte_index);
  }
  return word;
}

}  // namespace

bool StagedImageInstaller::AcceptsStagedImage(
    const StagedImageDescription& announced) const noexcept {
  const auto accepted = AcceptStagedContainer(slot_.StagedContainer(), constraints_);
  if (!accepted.has_value()) {
    return false;
  }

  return accepted->header.payload_crc32 == announced.payload_crc32 &&
         accepted->header.payload_size_bytes == announced.payload_size_bytes &&
         accepted->header.product_id == announced.product_id;
}

bool StagedImageInstaller::ApplicationSlotHoldsBootableImage() const noexcept {
  const auto slot = slot_.ApplicationSlot();
  if (slot.size() < kVectorTableProbeSizeBytes) {
    return false;
  }

  const std::uint32_t stack_pointer = ReadWord(slot, kStackPointerWordIndex);
  const std::uint32_t reset_handler = ReadWord(slot, kResetHandlerWordIndex);

  const std::uint32_t stack_region = stack_pointer & kAddressRegionMask;
  const bool stack_pointer_is_in_ram =
      stack_region == kDtcmRamRegion || stack_region == kAxiSramRegion;

  const std::uint32_t slot_start = constraints_.expected_load_address;
  const std::uint32_t slot_end = slot_start + constraints_.maximum_payload_size_bytes;
  const bool reset_handler_is_in_slot = reset_handler >= slot_start && reset_handler < slot_end;

  return stack_pointer_is_in_ram && reset_handler_is_in_slot;
}

InstallOutcome StagedImageInstaller::Install() noexcept {
  const auto accepted = AcceptStagedContainer(slot_.StagedContainer(), constraints_);
  if (!accepted.has_value()) {
    return InstallOutcome::kStagedImageRejected;
  }

  if (!slot_.EraseApplicationSlot(accepted->payload.size())) {
    return InstallOutcome::kEraseFailed;
  }

  if (!slot_.ProgramApplicationSlot(accepted->payload)) {
    return InstallOutcome::kProgramFailed;
  }

  const auto installed = slot_.ApplicationSlot().first(accepted->payload.size());
  if (checksum::ComputeCrc32(installed) != accepted->header.payload_crc32) {
    return InstallOutcome::kVerificationFailed;
  }

  return InstallOutcome::kInstalled;
}

}  // namespace midismith::image_installer

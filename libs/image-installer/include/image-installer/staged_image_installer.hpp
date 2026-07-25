#pragma once

#include <cstdint>

#include "firmware-image/image_installability.hpp"
#include "image-installer/application_slot_requirements.hpp"

namespace midismith::image_installer {

enum class InstallOutcome : std::uint8_t {
  kInstalled = 0,
  kStagedImageRejected,
  kEraseFailed,
  kProgramFailed,
  kVerificationFailed,
};

struct StagedImageDescription {
  std::uint32_t payload_crc32 = 0;
  std::uint32_t payload_size_bytes = 0;
  product_id::ProductId product_id = product_id::ProductId::kUnknown;

  bool operator==(const StagedImageDescription&) const = default;
};

class StagedImageInstaller {
 public:
  StagedImageInstaller(ApplicationSlotRequirements& slot,
                       const firmware_image::TargetConstraints& constraints) noexcept
      : slot_(slot), constraints_(constraints) {}

  [[nodiscard]] bool AcceptsStagedImage(const StagedImageDescription& announced) const noexcept;

  [[nodiscard]] bool ApplicationSlotHoldsBootableImage() const noexcept;

  [[nodiscard]] InstallOutcome Install() noexcept;

 private:
  ApplicationSlotRequirements& slot_;
  firmware_image::TargetConstraints constraints_;
};

}  // namespace midismith::image_installer

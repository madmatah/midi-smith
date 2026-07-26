#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "firmware-image/image_header.hpp"
#include "firmware-image/image_installability.hpp"
#include "firmware-staging/staging_slot_requirements.hpp"

namespace midismith::firmware_staging {

enum class StagingOutcome : std::uint8_t {
  kStaged = 0,
  kContainerDoesNotFitTheSlot,
  kEraseFailed,
  kProgramFailed,
  kMoreBytesThanAnnounced,
  kFewerBytesThanAnnounced,
  kStagedContainerRejected,
  kWriterNotStarted,
};

class StagingWriter {
 public:
  explicit StagingWriter(StagingSlotRequirements& slot) noexcept : slot_(slot) {}

  [[nodiscard]] StagingOutcome Begin(std::size_t container_size_bytes) noexcept;

  [[nodiscard]] StagingOutcome Write(std::span<const std::uint8_t> chunk) noexcept;

  [[nodiscard]] StagingOutcome Finish(
      const firmware_image::TargetConstraints& constraints) noexcept;

  [[nodiscard]] std::size_t accepted_bytes() const noexcept {
    return accepted_bytes_;
  }

 private:
  StagingSlotRequirements& slot_;
  std::size_t announced_size_bytes_ = 0;
  std::size_t accepted_bytes_ = 0;
  std::size_t programmed_bytes_ = 0;
  alignas(sizeof(
      std::uint32_t)) std::array<std::uint8_t, firmware_image::kFlashWordSizeBytes> pending_word_{};
  std::size_t pending_bytes_ = 0;
  bool started_ = false;
};

}  // namespace midismith::firmware_staging

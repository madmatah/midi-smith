#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include "app/shell/self_update_requirements.hpp"
#include "app/update/self_update_outcome.hpp"
#include "boot-control/boot_journal_writer.hpp"
#include "firmware-staging/staging_slot_requirements.hpp"
#include "firmware-staging/staging_writer.hpp"
#include "update-catalogue/image_source_requirements.hpp"

namespace midismith::main_board::app::update {

class SelfUpdateService final : public midismith::main_board::app::shell::SelfUpdateRequirements {
 public:
  SelfUpdateService(midismith::update_catalogue::ImageSourceRequirements& images,
                    midismith::firmware_staging::StagingSlotRequirements& staging,
                    midismith::boot_control::BootJournalWriter& journal,
                    std::string_view installed_version) noexcept;

  [[nodiscard]] SelfUpdateOutcome Run() noexcept override;

  [[nodiscard]] midismith::firmware_staging::StagingOutcome last_staging_outcome() const noexcept {
    return last_staging_outcome_;
  }

 private:
  static constexpr std::size_t kCopyBufferSizeBytes = 2048;

  midismith::update_catalogue::ImageSourceRequirements& images_;
  midismith::firmware_staging::StagingSlotRequirements& staging_;
  midismith::boot_control::BootJournalWriter& journal_;
  std::string_view installed_version_;
  std::array<std::uint8_t, kCopyBufferSizeBytes> copy_buffer_{};
  midismith::firmware_staging::StagingOutcome last_staging_outcome_ =
      midismith::firmware_staging::StagingOutcome::kWriterNotStarted;
};

}  // namespace midismith::main_board::app::update

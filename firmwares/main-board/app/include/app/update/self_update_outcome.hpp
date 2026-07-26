#pragma once

#include <cstdint>

namespace midismith::main_board::app::update {

enum class SelfUpdateOutcome : std::uint8_t {
  kStagedAndPending = 0,
  kNoImageOnCard,
  kImageUnusable,
  kAlreadyRunningThisBuild,
  kStagingFailed,
  kJournalWriteFailed,
};

}  // namespace midismith::main_board::app::update

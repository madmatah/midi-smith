#pragma once

#include <cstdint>

namespace midismith::bsp::storage {

enum class SdCardBringUpOutcome : std::uint8_t {
  kNeverAttempted = 0,
  kReady,
  kNoCardAnswered,
  kWideBusRefused,
};

}  // namespace midismith::bsp::storage

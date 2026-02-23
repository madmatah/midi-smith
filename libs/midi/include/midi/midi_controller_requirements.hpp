#pragma once

#include <cstdint>

namespace midismith::midi {

class MidiControllerRequirements {
 public:
  virtual ~MidiControllerRequirements() = default;

  virtual void SendRawMessage(const uint8_t* data, uint8_t length) noexcept = 0;
};

}  // namespace midismith::midi

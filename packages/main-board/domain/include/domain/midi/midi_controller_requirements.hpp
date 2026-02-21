#pragma once

#include <cstdint>

namespace domain::midi {

class MidiControllerRequirements {
 public:
  virtual ~MidiControllerRequirements() = default;

  virtual void SendRawMessage(const uint8_t* data, uint8_t length) noexcept = 0;
};

}  // namespace domain::midi

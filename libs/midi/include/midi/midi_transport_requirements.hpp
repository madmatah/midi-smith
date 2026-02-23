#pragma once

#include <cstdint>

#include "midi/midi_controller_requirements.hpp"
#include "midi/transport_status.hpp"

namespace midismith::midi {

class MidiTransportRequirements : public MidiControllerRequirements {
 public:
  virtual ~MidiTransportRequirements() = default;

  virtual bool IsAvailable() const noexcept = 0;

  virtual TransportStatus TrySendRawMessage(const uint8_t* data, uint8_t length) noexcept = 0;
};

}  // namespace midismith::midi

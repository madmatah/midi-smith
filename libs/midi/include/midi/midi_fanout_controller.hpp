#pragma once

#include <cstddef>
#include <cstdint>

#include "midi/midi_controller_requirements.hpp"

namespace midismith::midi {

class MidiFanoutController : public MidiControllerRequirements {
 public:
  MidiFanoutController(MidiControllerRequirements* const* sinks, std::size_t sink_count) noexcept;

  void SendRawMessage(const uint8_t* data, uint8_t length) noexcept override;

 private:
  MidiControllerRequirements* const* sinks_;
  std::size_t sink_count_;
};

}  // namespace midismith::midi

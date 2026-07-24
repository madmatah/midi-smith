#pragma once

#include <cstdint>

#include "midi-monitor/midi_activity_recorder_requirements.hpp"
#include "midi-monitor/midi_activity_source.hpp"
#include "midi/midi_controller_requirements.hpp"

namespace midismith::midi_monitor {

class MidiActivityTap final : public midismith::midi::MidiControllerRequirements {
 public:
  MidiActivityTap(midismith::midi::MidiControllerRequirements& sink,
                  MidiActivityRecorderRequirements& recorder, MidiActivitySource source) noexcept;

  void SendRawMessage(const std::uint8_t* data, std::uint8_t length) noexcept override;

 private:
  midismith::midi::MidiControllerRequirements& sink_;
  MidiActivityRecorderRequirements& recorder_;
  MidiActivitySource source_;
};

}  // namespace midismith::midi_monitor

#pragma once

#include <cstdint>

#include "midi-monitor/midi_activity_source.hpp"

namespace midismith::midi_monitor {

class MidiActivityRecorderRequirements {
 public:
  virtual ~MidiActivityRecorderRequirements() = default;

  virtual void RecordMessage(MidiActivitySource source, const std::uint8_t* data,
                             std::uint8_t length) noexcept = 0;
};

}  // namespace midismith::midi_monitor

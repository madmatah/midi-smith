#include "midi-monitor/midi_activity_tap.hpp"

namespace midismith::midi_monitor {

MidiActivityTap::MidiActivityTap(midismith::midi::MidiControllerRequirements& sink,
                                 MidiActivityRecorderRequirements& recorder,
                                 MidiActivitySource source) noexcept
    : sink_(sink), recorder_(recorder), source_(source) {}

void MidiActivityTap::SendRawMessage(const std::uint8_t* data, std::uint8_t length) noexcept {
  sink_.SendRawMessage(data, length);
  recorder_.RecordMessage(source_, data, length);
}

}  // namespace midismith::midi_monitor

#pragma once

#include <array>
#include <cstdint>

#include "midi-monitor/midi_activity_source.hpp"
#include "midi/types.hpp"

namespace midismith::midi_monitor {

struct MidiActivitySnapshot {
  bool has_last_note = false;
  midismith::midi::NoteNumber last_note_number = 0;
  midismith::midi::Velocity last_note_velocity = 0;
  std::uint8_t last_note_channel = 0;
  std::uint16_t last_note_sequence = 0;
  std::uint8_t active_note_count = 0;
  std::array<std::uint32_t, kMidiActivitySourceCount> message_counts{};
  std::uint32_t total_message_count = 0;

  std::uint32_t message_count(MidiActivitySource source) const noexcept {
    return message_counts[IndexOf(source)];
  }
};

}  // namespace midismith::midi_monitor

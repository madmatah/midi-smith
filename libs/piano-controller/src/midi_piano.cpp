#include "piano-controller/midi_piano.hpp"

#include <cstdint>

namespace midismith::piano_controller {

MidiPiano::MidiPiano(midismith::midi::MidiControllerRequirements& midi_controller,
                     const Config& config) noexcept
    : midi_controller_(midi_controller), config_(config) {
  config_.channel &= 0x0F;
}

void MidiPiano::PressKey(midismith::midi::NoteNumber note,
                         midismith::midi::Velocity velocity) noexcept {
  uint8_t message[3] = {static_cast<uint8_t>(0x90 | config_.channel), note, velocity};
  midi_controller_.SendRawMessage(message, sizeof(message));
}

void MidiPiano::ReleaseKey(midismith::midi::NoteNumber note) noexcept {
  uint8_t message[3] = {static_cast<uint8_t>(0x80 | config_.channel), note, 0};
  midi_controller_.SendRawMessage(message, sizeof(message));
}

void MidiPiano::SetSustain(bool active) noexcept {
  uint8_t message[3] = {static_cast<uint8_t>(0xB0 | config_.channel), config_.sustain_cc,
                        static_cast<uint8_t>(active ? 127 : 0)};
  midi_controller_.SendRawMessage(message, sizeof(message));
}

void MidiPiano::SetSoft(bool active) noexcept {
  uint8_t message[3] = {static_cast<uint8_t>(0xB0 | config_.channel), config_.soft_cc,
                        static_cast<uint8_t>(active ? 127 : 0)};
  midi_controller_.SendRawMessage(message, sizeof(message));
}

void MidiPiano::SetSostenuto(bool active) noexcept {
  uint8_t message[3] = {static_cast<uint8_t>(0xB0 | config_.channel), config_.sostenuto_cc,
                        static_cast<uint8_t>(active ? 127 : 0)};
  midi_controller_.SendRawMessage(message, sizeof(message));
}

}  // namespace midismith::piano_controller

#pragma once

#include <cstdint>

#include "domain/music/piano/piano_requirements.hpp"
#include "midi/midi_controller_requirements.hpp"

namespace midismith::main_board::domain::music::piano {

class MidiPiano : public PianoRequirements {
 public:
  struct Config {
    uint8_t channel;
    uint8_t sustain_cc;
    uint8_t soft_cc;
    uint8_t sostenuto_cc;
  };

  MidiPiano(midismith::midi::MidiControllerRequirements& midi_controller,
            const Config& config) noexcept;

  void PressKey(midismith::midi::NoteNumber note,
                midismith::midi::Velocity velocity) noexcept override;
  void ReleaseKey(midismith::midi::NoteNumber note) noexcept override;

  void SetSustain(bool active) noexcept override;
  void SetSoft(bool active) noexcept override;
  void SetSostenuto(bool active) noexcept override;

 private:
  midismith::midi::MidiControllerRequirements& midi_controller_;
  Config config_;
};

}  // namespace midismith::main_board::domain::music::piano

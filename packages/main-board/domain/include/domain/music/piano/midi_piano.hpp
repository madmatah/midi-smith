#pragma once

#include <cstdint>

#include "domain/midi/midi_controller_requirements.hpp"
#include "domain/music/piano/piano_requirements.hpp"

namespace midismith::main_board::domain::music::piano {

class MidiPiano : public PianoRequirements {
 public:
  struct Config {
    uint8_t channel;
    uint8_t sustain_cc;
    uint8_t soft_cc;
    uint8_t sostenuto_cc;
  };

  MidiPiano(midismith::main_board::domain::midi::MidiControllerRequirements& midi_controller,
            const Config& config) noexcept;

  void PressKey(midismith::common::domain::music::NoteNumber note,
                midismith::common::domain::music::Velocity velocity) noexcept override;
  void ReleaseKey(midismith::common::domain::music::NoteNumber note) noexcept override;

  void SetSustain(bool active) noexcept override;
  void SetSoft(bool active) noexcept override;
  void SetSostenuto(bool active) noexcept override;

 private:
  midismith::main_board::domain::midi::MidiControllerRequirements& midi_controller_;
  Config config_;
};

}  // namespace midismith::main_board::domain::music::piano

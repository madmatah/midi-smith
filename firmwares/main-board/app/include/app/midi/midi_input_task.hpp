#pragma once

#include "io/stream_requirements.hpp"
#include "midi/midi_controller_requirements.hpp"
#include "midi/midi_input_parser.hpp"
#include "os-types/binary_semaphore_requirements.hpp"

namespace midismith::main_board::app::midi {

class MidiInputTask {
 public:
  MidiInputTask(midismith::io::ReadableStreamRequirements& source,
                midismith::midi::MidiControllerRequirements& sink,
                midismith::os::BinarySemaphoreRequirements& wake_signal) noexcept;

  void Run() noexcept;

 private:
  void DrainSource() noexcept;

  midismith::io::ReadableStreamRequirements& source_;
  midismith::midi::MidiInputParser parser_;
  midismith::os::BinarySemaphoreRequirements& wake_signal_;
};

}  // namespace midismith::main_board::app::midi

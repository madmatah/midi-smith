#include "app/midi/midi_input_task.hpp"

#include <cstdint>

#include "os-types/queue_requirements.hpp"

namespace midismith::main_board::app::midi {

MidiInputTask::MidiInputTask(midismith::io::ReadableStreamRequirements& source,
                             midismith::midi::MidiControllerRequirements& sink,
                             midismith::os::BinarySemaphoreRequirements& wake_signal) noexcept
    : source_(source), parser_(sink), wake_signal_(wake_signal) {}

void MidiInputTask::Run() noexcept {
  while (wake_signal_.Acquire(midismith::os::kWaitForever)) {
    DrainSource();
  }
}

void MidiInputTask::DrainSource() noexcept {
  std::uint8_t byte;
  while (source_.Read(byte) == midismith::io::ReadResult::kOk) {
    parser_.Feed(byte);
  }
}

}  // namespace midismith::main_board::app::midi

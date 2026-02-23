#pragma once

#include "app/midi/midi_command.hpp"
#include "midi/midi_controller_requirements.hpp"
#include "os/queue_requirements.hpp"

namespace midismith::main_board::app::midi {

/**
 * @brief Asynchronous MIDI controller that forwards messages to a queue.
 *
 * This implementation is non-blocking (timeout = 0) to ensure the caller
 * (e.g., UI or Logic task) is never delayed by MIDI transport.
 */
class AsyncTaskMidiController : public midismith::midi::MidiControllerRequirements {
 public:
  explicit AsyncTaskMidiController(midismith::common::os::QueueRequirements<MidiCommand>& queue)
      : _queue(queue) {}

  /**
   * @brief Formats and posts a MIDI message to the task queue.
   */
  void SendRawMessage(const uint8_t* data, uint8_t length) noexcept override;

 private:
  midismith::common::os::QueueRequirements<MidiCommand>& _queue;
};

}  // namespace midismith::main_board::app::midi

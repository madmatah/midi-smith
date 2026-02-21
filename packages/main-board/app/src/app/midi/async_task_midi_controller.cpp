#include "app/midi/async_task_midi_controller.hpp"

#include <cstring>

namespace app::midi {

void AsyncTaskMidiController::SendRawMessage(const uint8_t* data, uint8_t length) noexcept {
  if (data == nullptr || length == 0 || length > 3) {
    return;
  }

  MidiCommand command{};
  std::memcpy(command.data, data, length);
  command.length = length;

  (void) _queue.Send(command, os::kNoWait);
}

}  // namespace app::midi

#include "app/midi/midi_task.hpp"

#include "os/clock.hpp"

namespace midismith::main_board::app::midi {

[[noreturn]] void MidiTask::Run() noexcept {
  while (true) {
    MidiCommand command{};
    if (_queue.Receive(command, midismith::os::kWaitForever)) {
      ProcessCommand(command);
    }
  }
}

void MidiTask::ProcessCommand(const MidiCommand& command) noexcept {
  if (_transport.IsAvailable()) {
    TransmitWithRetry(command);
  }
}

void MidiTask::TransmitWithRetry(const MidiCommand& command) noexcept {
  uint32_t elapsed_ms = 0;
  const uint32_t kRetryIntervalMs = 1;

  while (elapsed_ms <= _retry_timeout_ms) {
    const auto status = _transport.TrySendRawMessage(command.data, command.length);

    if (status == midismith::midi::TransportStatus::kSuccess) {
      return;
    }

    if (status == midismith::midi::TransportStatus::kBusy) {
      midismith::os::Clock::delay_ms(kRetryIntervalMs);
      elapsed_ms += kRetryIntervalMs;
      continue;
    }

    if (status == midismith::midi::TransportStatus::kError) {
      _logger.logf(midismith::logging::Level::Error, "MidiTask: Transport error\n");
      return;
    }

    if (status == midismith::midi::TransportStatus::kNotAvailable) {
      return;
    }
  }

  _logger.logf(midismith::logging::Level::Warn, "MidiTask: Drop message (timeout)\n");
}

}  // namespace midismith::main_board::app::midi

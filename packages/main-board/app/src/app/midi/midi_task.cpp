#include "app/midi/midi_task.hpp"

#include "os/clock.hpp"

namespace app::midi {

[[noreturn]] void MidiTask::Run() noexcept {
  while (true) {
    MidiCommand command{};
    if (_queue.Receive(command, os::kWaitForever)) {
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

    if (status == domain::midi::TransportStatus::kSuccess) {
      return;
    }

    if (status == domain::midi::TransportStatus::kBusy) {
      os::Clock::delay_ms(kRetryIntervalMs);
      elapsed_ms += kRetryIntervalMs;
      continue;
    }

    if (status == domain::midi::TransportStatus::kError) {
      _logger.logf(app::Logging::Level::Error, "MidiTask: Transport error");
      return;
    }

    if (status == domain::midi::TransportStatus::kNotAvailable) {
      return;
    }
  }

  _logger.logf(app::Logging::Level::Warn, "MidiTask: Drop message (timeout)");
}

}  // namespace app::midi

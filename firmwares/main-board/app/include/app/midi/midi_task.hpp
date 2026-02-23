#pragma once

#include "app/midi/midi_command.hpp"
#include "logging/logger_requirements.hpp"
#include "midi/midi_transport_requirements.hpp"
#include "os/queue_requirements.hpp"

namespace midismith::main_board::app::midi {

/**
 * @brief FreeRTOS task responsible for processing and sending MIDI messages.
 *
 * It consumes messages from a queue and forwards them to a transport (e.g., USB).
 * It handles backpressure (busy transport) with retries and handles disconnects.
 */
class MidiTask {
 public:
  /**
   * @brief Constructor.
   * @param queue Message source.
   * @param transport Hardware/OS transport.
   * @param logger For error reporting.
   * @param retry_timeout_ms Max time to retry sending if transport is busy.
   */
  MidiTask(midismith::common::os::QueueRequirements<MidiCommand>& queue,
           midismith::midi::MidiTransportRequirements& transport,
           midismith::logging::LoggerRequirements& logger, uint32_t retry_timeout_ms = 100) noexcept
      : _queue(queue),
        _transport(transport),
        _logger(logger),
        _retry_timeout_ms(retry_timeout_ms) {}

  /**
   * @brief Main task loop.
   */
  [[noreturn]] void Run() noexcept;

 private:
  void ProcessCommand(const MidiCommand& command) noexcept;
  void TransmitWithRetry(const MidiCommand& command) noexcept;

  midismith::common::os::QueueRequirements<MidiCommand>& _queue;
  midismith::midi::MidiTransportRequirements& _transport;
  midismith::logging::LoggerRequirements& _logger;
  uint32_t _retry_timeout_ms;
};

}  // namespace midismith::main_board::app::midi

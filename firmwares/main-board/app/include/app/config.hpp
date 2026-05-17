#pragma once

#include <cstdint>

/**
 * @brief Centralized configuration for the application.
 */
namespace midismith::main_board::app::config {

// Task priorities
constexpr uint32_t SHELL_TASK_PRIORITY = 1;
constexpr uint32_t MIDI_TASK_PRIORITY = 3;
constexpr uint32_t MIDI_INPUT_TASK_PRIORITY = 3;

// Stack sizes
constexpr uint32_t MIDI_TASK_STACK_BYTES = 2048;
constexpr uint32_t MIDI_INPUT_TASK_STACK_BYTES = 1024;

// Shell
constexpr uint32_t SHELL_TASK_STACK_BYTES = 2048;
constexpr uint32_t SHELL_TASK_IDLE_DELAY_MS = 10;
constexpr std::size_t kConsoleUartRxBufferSize = 256;
constexpr std::size_t kConsoleUartTxFifoSize = 1024;

// Queue capacities
constexpr uint32_t USB_MIDI_QUEUE_CAPACITY = 64;
constexpr uint32_t DIN_MIDI_QUEUE_CAPACITY = 64;

// Timings
constexpr uint32_t LED_BLINK_PERIOD_MS = 500;
constexpr uint32_t MIDI_RETRY_TIMEOUT_MS = 5;

// RTT Telemetry
constexpr uint32_t RTT_TELEMETRY_LED_CHANNEL = 1;
constexpr uint32_t RTT_TELEMETRY_LED_BUFFER_SIZE = 256;

}  // namespace midismith::main_board::app::config

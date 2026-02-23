#include "app/application.hpp"

#include <cstdint>
#include <new>

#include "app/config.hpp"
#include "app/midi/async_task_midi_controller.hpp"
#include "app/midi/midi_command.hpp"
#include "app/midi/midi_task.hpp"
#include "app/tasks/led_task.hpp"
#include "bsp/board.hpp"
#include "bsp/rtt_logger.hpp"
#include "bsp/rtt_telemetry_sender.hpp"
#include "bsp/usb_midi.hpp"
#include "os/queue.hpp"
#include "os/task.hpp"
#include "piano-controller/midi_piano.hpp"

namespace midismith::main_board::app {

// Helper to run the MidiTask loop from a static function
static void midi_task_entry(void* ctx) noexcept {
  auto* task = reinterpret_cast<midismith::main_board::app::midi::MidiTask*>(ctx);
  task->Run();
}

void Application::init() noexcept {
  midismith::main_board::bsp::Board::init();
}

void Application::create_tasks() noexcept {
  static midismith::common::bsp::RttLogger logger;

  // 1. MIDI System
  static midismith::os::Queue<midismith::main_board::app::midi::MidiCommand,
                              midismith::main_board::app::config::MIDI_QUEUE_CAPACITY>
      midi_queue;
  static midismith::main_board::bsp::UsbMidi usb_midi;
  static midismith::main_board::app::midi::AsyncTaskMidiController midi_controller(midi_queue);
  static midismith::main_board::app::midi::MidiTask midi_task(
      midi_queue, usb_midi, logger, midismith::main_board::app::config::MIDI_RETRY_TIMEOUT_MS);

  static midismith::piano_controller::MidiPiano::Config piano_config = {
      .channel = 0, .sustain_cc = 64, .soft_cc = 67, .sostenuto_cc = 66};
  static midismith::piano_controller::MidiPiano piano(midi_controller, piano_config);

  // Start MIDI Task
  (void) midismith::os::Task::create("MidiTask", midi_task_entry, &midi_task,
                                     midismith::main_board::app::config::MIDI_TASK_STACK_BYTES,
                                     midismith::main_board::app::config::MIDI_TASK_PRIORITY);

  // 2. LED Task
  alignas(midismith::main_board::app::tasks::LedTask) static std::uint8_t
      led_task_storage[sizeof(midismith::main_board::app::tasks::LedTask)];
  static bool led_constructed = false;

  auto& led = midismith::main_board::bsp::Board::user_led();

  alignas(4) static std::uint8_t
      led_telemetry_buffer[midismith::main_board::app::config::RTT_TELEMETRY_LED_BUFFER_SIZE];
  static midismith::main_board::bsp::RttTelemetrySender telemetry(
      midismith::main_board::app::config::RTT_TELEMETRY_LED_CHANNEL, "LedTelemetry",
      led_telemetry_buffer, sizeof(led_telemetry_buffer));

  midismith::main_board::app::tasks::LedTask* led_task_ptr = nullptr;
  if (!led_constructed) {
    led_task_ptr =
        new (led_task_storage) midismith::main_board::app::tasks::LedTask(led, telemetry, piano);
    led_constructed = true;
  } else {
    led_task_ptr = reinterpret_cast<midismith::main_board::app::tasks::LedTask*>(led_task_storage);
  }

  (void) led_task_ptr->start();
}

}  // namespace midismith::main_board::app

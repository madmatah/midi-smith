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
#include "domain/music/piano/midi_piano.hpp"
#include "os/queue.hpp"
#include "os/task.hpp"

namespace app {

// Helper to run the MidiTask loop from a static function
static void midi_task_entry(void* ctx) noexcept {
  auto* task = reinterpret_cast<app::midi::MidiTask*>(ctx);
  task->Run();
}

void Application::init() noexcept {
  bsp::Board::init();
}

void Application::create_tasks() noexcept {
  static bsp::RttLogger logger;

  // 1. MIDI System
  static os::Queue<app::midi::MidiCommand, app::config::MIDI_QUEUE_CAPACITY> midi_queue;
  static bsp::UsbMidi usb_midi;
  static app::midi::AsyncTaskMidiController midi_controller(midi_queue);
  static app::midi::MidiTask midi_task(midi_queue, usb_midi, logger,
                                       app::config::MIDI_RETRY_TIMEOUT_MS);

  static domain::music::piano::MidiPiano::Config piano_config = {
      .channel = 0, .sustain_cc = 64, .soft_cc = 67, .sostenuto_cc = 66};
  static domain::music::piano::MidiPiano piano(midi_controller, piano_config);

  // Start MIDI Task
  (void) os::Task::create("MidiTask", midi_task_entry, &midi_task,
                          app::config::MIDI_TASK_STACK_BYTES, app::config::MIDI_TASK_PRIORITY);

  // 2. LED Task
  alignas(app::Tasks::LedTask) static std::uint8_t led_task_storage[sizeof(app::Tasks::LedTask)];
  static bool led_constructed = false;

  auto& led = bsp::Board::user_led();

  alignas(4) static std::uint8_t led_telemetry_buffer[app::config::RTT_TELEMETRY_LED_BUFFER_SIZE];
  static bsp::RttTelemetrySender telemetry(app::config::RTT_TELEMETRY_LED_CHANNEL, "LedTelemetry",
                                           led_telemetry_buffer, sizeof(led_telemetry_buffer));

  app::Tasks::LedTask* led_task_ptr = nullptr;
  if (!led_constructed) {
    led_task_ptr = new (led_task_storage) app::Tasks::LedTask(led, telemetry, piano);
    led_constructed = true;
  } else {
    led_task_ptr = reinterpret_cast<app::Tasks::LedTask*>(led_task_storage);
  }

  (void) led_task_ptr->start();
}

}  // namespace app

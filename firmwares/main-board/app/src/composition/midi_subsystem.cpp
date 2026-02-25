#include "app/composition/subsystems.hpp"
#include "app/config.hpp"
#include "app/midi/async_task_midi_controller.hpp"
#include "app/midi/midi_command.hpp"
#include "app/midi/midi_task.hpp"
#include "bsp/usb_midi.hpp"
#include "os/queue.hpp"
#include "os/task.hpp"
#include "piano-controller/midi_piano.hpp"

namespace midismith::main_board::app::composition {

namespace {

void midi_task_entry(void* ctx) noexcept {
  auto* task = reinterpret_cast<midismith::main_board::app::midi::MidiTask*>(ctx);
  if (task != nullptr) {
    task->Run();
  }
}

}  // namespace

MidiContext CreateMidiSubsystem(midismith::logging::LoggerRequirements& logger) noexcept {
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

  (void) midismith::os::Task::create("MidiTask", midi_task_entry, &midi_task,
                                     midismith::main_board::app::config::MIDI_TASK_STACK_BYTES,
                                     midismith::main_board::app::config::MIDI_TASK_PRIORITY);

  return MidiContext{piano};
}

}  // namespace midismith::main_board::app::composition

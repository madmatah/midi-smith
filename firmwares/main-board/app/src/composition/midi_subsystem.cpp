#include "app/composition/subsystems.hpp"
#include "app/config.hpp"
#include "app/midi/async_task_midi_controller.hpp"
#include "app/midi/midi_command.hpp"
#include "app/midi/midi_input_task.hpp"
#include "app/midi/midi_output_task.hpp"
#include "bsp/memory_sections.hpp"
#include "bsp/usart_midi.hpp"
#include "bsp/usb_midi.hpp"
#include "midi/midi_fanout_controller.hpp"
#include "os/binary_semaphore.hpp"
#include "os/queue.hpp"
#include "os/task.hpp"
#include "piano-controller/midi_piano.hpp"
#include "usart.h"

namespace midismith::main_board::app::composition {

namespace {

void midi_output_task_entry(void* ctx) noexcept {
  auto* task = reinterpret_cast<midismith::main_board::app::midi::MidiOutputTask*>(ctx);
  if (task != nullptr) {
    task->Run();
  }
}

void midi_input_task_entry(void* ctx) noexcept {
  auto* task = reinterpret_cast<midismith::main_board::app::midi::MidiInputTask*>(ctx);
  if (task != nullptr) {
    task->Run();
  }
}

void release_semaphore_from_isr(void* ctx) noexcept {
  static_cast<midismith::os::BinarySemaphoreRequirements*>(ctx)->Release();
}

}  // namespace

MidiContext CreateMidiSubsystem(midismith::logging::LoggerRequirements& logger) noexcept {
  static midismith::os::Queue<midismith::main_board::app::midi::MidiCommand,
                              midismith::main_board::app::config::USB_MIDI_QUEUE_CAPACITY>
      usb_midi_queue;
  static midismith::main_board::bsp::UsbMidi usb_midi;
  static midismith::main_board::app::midi::AsyncTaskMidiController usb_midi_controller(
      usb_midi_queue);
  static midismith::main_board::app::midi::MidiOutputTask usb_midi_task(
      usb_midi_queue, usb_midi, logger, midismith::main_board::app::config::MIDI_RETRY_TIMEOUT_MS);

  static midismith::os::Queue<midismith::main_board::app::midi::MidiCommand,
                              midismith::main_board::app::config::DIN_MIDI_QUEUE_CAPACITY>
      din_midi_queue;
  alignas(32) BSP_AXI_SRAM_NOCACHE static midismith::main_board::bsp::UsartMidi din_midi(huart3);
  static midismith::main_board::app::midi::AsyncTaskMidiController din_midi_controller(
      din_midi_queue);
  static midismith::main_board::app::midi::MidiOutputTask din_midi_task(
      din_midi_queue, din_midi, logger, midismith::main_board::app::config::MIDI_RETRY_TIMEOUT_MS);

  static midismith::midi::MidiControllerRequirements* fanout_sinks[] = {&usb_midi_controller,
                                                                        &din_midi_controller};
  static midismith::midi::MidiFanoutController midi_fanout(fanout_sinks, 2);

  static midismith::piano_controller::MidiPiano::Config piano_config = {
      .channel = 0, .sustain_cc = 64, .soft_cc = 67, .sostenuto_cc = 66};
  static midismith::piano_controller::MidiPiano piano(midi_fanout, piano_config);

  static midismith::os::BinarySemaphore din_midi_input_wake;
  din_midi.SetByteAvailableCallback(release_semaphore_from_isr, &din_midi_input_wake);
  (void) din_midi.StartReception();
  static midismith::main_board::app::midi::MidiInputTask midi_input_task(din_midi, midi_fanout,
                                                                         din_midi_input_wake);

  (void) midismith::os::Task::create("UsbMidiInputTask", midi_output_task_entry, &usb_midi_task,
                                     midismith::main_board::app::config::MIDI_TASK_STACK_BYTES,
                                     midismith::main_board::app::config::MIDI_TASK_PRIORITY);
  (void) midismith::os::Task::create("DinMidiInputTask", midi_output_task_entry, &din_midi_task,
                                     midismith::main_board::app::config::MIDI_TASK_STACK_BYTES,
                                     midismith::main_board::app::config::MIDI_TASK_PRIORITY);
  (void) midismith::os::Task::create(
      "MidiInputTask", midi_input_task_entry, &midi_input_task,
      midismith::main_board::app::config::MIDI_INPUT_TASK_STACK_BYTES,
      midismith::main_board::app::config::MIDI_INPUT_TASK_PRIORITY);

  return MidiContext{piano};
}

}  // namespace midismith::main_board::app::composition

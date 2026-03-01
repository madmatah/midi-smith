#include "app/messaging/main_board_inbound_sensor_event_midi_handler.hpp"

namespace midismith::main_board::app::messaging {

namespace {

constexpr midismith::midi::NoteNumber kFixedNoteNumber = 64;

}  // namespace

void MainBoardInboundSensorEventMidiHandler::OnSensorEvent(
    const midismith::protocol::SensorEvent& event) noexcept {
  switch (event.type) {
    case midismith::protocol::SensorEventType::kNoteOn:
      piano_.PressKey(kFixedNoteNumber, event.velocity);
      return;

    case midismith::protocol::SensorEventType::kNoteOff:
      piano_.ReleaseKey(kFixedNoteNumber, event.velocity);
      return;
  }
}

}  // namespace midismith::main_board::app::messaging

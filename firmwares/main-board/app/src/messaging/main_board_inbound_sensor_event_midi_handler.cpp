#include "app/messaging/main_board_inbound_sensor_event_midi_handler.hpp"

namespace midismith::main_board::app::messaging {

void MainBoardInboundSensorEventMidiHandler::OnSensorEvent(
    const midismith::protocol::SensorEvent& event, std::uint8_t source_node_id) noexcept {
  auto midi_note = keymap_lookup_.FindMidiNote(source_node_id, event.sensor_id);
  if (!midi_note.has_value()) {
    return;
  }

  switch (event.type) {
    case midismith::protocol::SensorEventType::kNoteOn:
      piano_.PressKey(*midi_note, event.velocity);
      return;

    case midismith::protocol::SensorEventType::kNoteOff:
      piano_.ReleaseKey(*midi_note, event.velocity);
      return;
  }
}

}  // namespace midismith::main_board::app::messaging

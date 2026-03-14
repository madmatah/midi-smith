#include "app/messaging/main_board_inbound_keymap_capture_handler.hpp"

namespace midismith::main_board::app::messaging {

void MainBoardInboundKeymapCaptureHandler::OnSensorEvent(
    const midismith::protocol::SensorEvent& event, std::uint8_t source_node_id) noexcept {
  if (event.type != midismith::protocol::SensorEventType::kNoteOn) {
    return;
  }
  coordinator_.TryCaptureNoteOn(source_node_id, event.sensor_id);
}

}  // namespace midismith::main_board::app::messaging

#include "app/messaging/main_board_inbound_sensor_event_logging_handler.hpp"

namespace midismith::main_board::app::messaging {

void MainBoardInboundSensorEventLoggingHandler::OnSensorEvent(
    const midismith::protocol::SensorEvent& event, std::uint8_t source_node_id) noexcept {
  switch (event.type) {
    case midismith::protocol::SensorEventType::kNoteOn:
      logger_.infof("noteon:%u:%u:%u\n", static_cast<unsigned>(source_node_id),
                    static_cast<unsigned>(event.sensor_id), static_cast<unsigned>(event.velocity));
      return;

    case midismith::protocol::SensorEventType::kNoteOff:
      logger_.infof("noteoff:%u:%u:%u\n", static_cast<unsigned>(source_node_id),
                    static_cast<unsigned>(event.sensor_id), static_cast<unsigned>(event.velocity));
      return;
  }
}

}  // namespace midismith::main_board::app::messaging

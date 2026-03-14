#pragma once

#include "app/keymap/keymap_setup_coordinator.hpp"
#include "protocol/messages.hpp"

namespace midismith::main_board::app::messaging {

class MainBoardInboundKeymapCaptureHandler {
 public:
  explicit MainBoardInboundKeymapCaptureHandler(
      midismith::main_board::app::keymap::KeymapSetupCoordinator& coordinator) noexcept
      : coordinator_(coordinator) {}

  void OnSensorEvent(const midismith::protocol::SensorEvent& event,
                     std::uint8_t source_node_id) noexcept;

 private:
  midismith::main_board::app::keymap::KeymapSetupCoordinator& coordinator_;
};

}  // namespace midismith::main_board::app::messaging

#pragma once

#include "piano-controller/piano_requirements.hpp"
#include "protocol/messages.hpp"

namespace midismith::main_board::app::messaging {

class MainBoardInboundSensorEventMidiHandler final {
 public:
  explicit MainBoardInboundSensorEventMidiHandler(
      midismith::piano_controller::PianoRequirements& piano) noexcept
      : piano_(piano) {}

  void OnSensorEvent(const midismith::protocol::SensorEvent& event) noexcept;

 private:
  midismith::piano_controller::PianoRequirements& piano_;
};

}  // namespace midismith::main_board::app::messaging

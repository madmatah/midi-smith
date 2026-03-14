#pragma once

#include <cstdint>

#include "domain/keymap/keymap_lookup_requirements.hpp"
#include "piano-controller/piano_requirements.hpp"
#include "protocol/messages.hpp"

namespace midismith::main_board::app::messaging {

class MainBoardInboundSensorEventMidiHandler final {
 public:
  MainBoardInboundSensorEventMidiHandler(
      midismith::piano_controller::PianoRequirements& piano,
      const midismith::main_board::domain::config::KeymapLookupRequirements& keymap_lookup) noexcept
      : piano_(piano), keymap_lookup_(keymap_lookup) {}

  void OnSensorEvent(const midismith::protocol::SensorEvent& event,
                     std::uint8_t source_node_id) noexcept;

 private:
  midismith::piano_controller::PianoRequirements& piano_;
  const midismith::main_board::domain::config::KeymapLookupRequirements& keymap_lookup_;
};

}  // namespace midismith::main_board::app::messaging

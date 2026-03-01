#pragma once

#include <cstdint>

#include "app/messaging/adc_board_message_sender_requirements.hpp"
#include "piano-sensing/key_action_requirements.hpp"

namespace midismith::adc_board::app::piano_sensing {

class RemoteMessageSensorEventHandler final
    : public midismith::piano_sensing::KeyActionRequirements {
 public:
  RemoteMessageSensorEventHandler(
      midismith::adc_board::app::messaging::AdcBoardMessageSenderRequirements& message_sender,
      std::uint8_t sensor_id) noexcept
      : message_sender_(message_sender), sensor_id_(sensor_id) {}

  void OnNoteOn(midismith::midi::Velocity velocity) noexcept override {
    (void) message_sender_.SendNoteOn(sensor_id_, velocity);
  }

  void OnNoteOff(midismith::midi::Velocity release_velocity) noexcept override {
    (void) message_sender_.SendNoteOff(sensor_id_, release_velocity);
  }

 private:
  midismith::adc_board::app::messaging::AdcBoardMessageSenderRequirements& message_sender_;
  std::uint8_t sensor_id_ = 0u;
};

}  // namespace midismith::adc_board::app::piano_sensing

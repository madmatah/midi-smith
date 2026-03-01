#pragma once

#include <cstdint>

#include "logging/logger_requirements.hpp"
#include "piano-sensing/key_action_requirements.hpp"

namespace midismith::adc_board::app::piano_sensing {

class LoggingSensorEventHandler final : public midismith::piano_sensing::KeyActionRequirements {
 public:
  LoggingSensorEventHandler(midismith::logging::LoggerRequirements& logger,
                            std::uint8_t sensor_id) noexcept
      : logger_(logger), sensor_id_(sensor_id) {}

  void OnNoteOn(midismith::midi::Velocity velocity) noexcept override {
    logger_.infof("noteon:%u:%u\n", static_cast<unsigned>(sensor_id_),
                  static_cast<unsigned>(velocity));
  }

  void OnNoteOff(midismith::midi::Velocity release_velocity) noexcept override {
    logger_.infof("noteoff:%u:%u\n", static_cast<unsigned>(sensor_id_),
                  static_cast<unsigned>(release_velocity));
  }

 private:
  midismith::logging::LoggerRequirements& logger_;
  std::uint8_t sensor_id_ = 0u;
};

}  // namespace midismith::adc_board::app::piano_sensing

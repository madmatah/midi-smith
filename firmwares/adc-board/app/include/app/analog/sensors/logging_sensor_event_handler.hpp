#pragma once

#include <cstdint>

#include "logging/logger_requirements.hpp"
#include "piano-sensing/key_action_requirements.hpp"

namespace midismith::adc_board::app::analog::sensors {

class LoggingSensorEventHandler final : public midismith::piano_sensing::KeyActionRequirements {
 public:
  void SetLogger(midismith::logging::LoggerRequirements* logger) noexcept {
    logger_ = logger;
  }

  void SetSensorId(std::uint8_t sensor_id) noexcept {
    sensor_id_ = sensor_id;
  }

  void OnNoteOn(midismith::midi::Velocity velocity) noexcept override {
    if (logger_ == nullptr) {
      return;
    }
    logger_->infof("noteon:%u:%u\n", static_cast<unsigned>(sensor_id_),
                   static_cast<unsigned>(velocity));
  }

  void OnNoteOff(midismith::midi::Velocity release_velocity) noexcept override {
    if (logger_ == nullptr) {
      return;
    }
    logger_->infof("noteoff:%u:%u\n", static_cast<unsigned>(sensor_id_),
                   static_cast<unsigned>(release_velocity));
  }

 private:
  midismith::logging::LoggerRequirements* logger_ = nullptr;
  std::uint8_t sensor_id_ = 0u;
};

}  // namespace midismith::adc_board::app::analog::sensors

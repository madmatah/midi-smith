#pragma once

#include <cstdint>

#include "app/logging/logger_requirements.hpp"
#include "domain/music/piano/key_action_requirements.hpp"

namespace app::analog::sensors {

class LoggingSensorEventHandler final : public domain::music::piano::KeyActionRequirements {
 public:
  void SetLogger(app::Logging::LoggerRequirements* logger) noexcept {
    logger_ = logger;
  }

  void SetSensorId(std::uint8_t sensor_id) noexcept {
    sensor_id_ = sensor_id;
  }

  void OnNoteOn(domain::music::Velocity velocity) noexcept override {
    if (logger_ == nullptr) {
      return;
    }
    logger_->infof("noteon:%u:%u\n", static_cast<unsigned>(sensor_id_),
                   static_cast<unsigned>(velocity));
  }

  void OnNoteOff(domain::music::Velocity release_velocity) noexcept override {
    if (logger_ == nullptr) {
      return;
    }
    logger_->infof("noteoff:%u:%u\n", static_cast<unsigned>(sensor_id_),
                   static_cast<unsigned>(release_velocity));
  }

 private:
  app::Logging::LoggerRequirements* logger_ = nullptr;
  std::uint8_t sensor_id_ = 0u;
};

}  // namespace app::analog::sensors

#pragma once

#include "calibration/calibration_validator_requirements.hpp"
#include "calibration/sensor_calibration.hpp"

namespace midismith::calibration {

class SensorCalibrationValidator final : public CalibrationValidatorRequirements {
 public:
  explicit SensorCalibrationValidator(float min_delta_ma, float max_strike_current_ma) noexcept
      : min_delta_ma_(min_delta_ma), max_strike_current_ma_(max_strike_current_ma) {}

  bool IsValidCalibration(const SensorCalibration& calib) const noexcept override {
    if (calib.rest_current_ma < 0.0f) return false;
    if ((calib.strike_current_ma - calib.rest_current_ma) <= min_delta_ma_) return false;
    if (calib.strike_current_ma > max_strike_current_ma_) return false;
    return true;
  }

 private:
  float min_delta_ma_;
  float max_strike_current_ma_;
};

}  // namespace midismith::calibration

#pragma once

#include "app/config/config.hpp"
#include "calibration/sensor_calibration.hpp"
#include "domain/calibration/calibration_data_validity_requirements.hpp"

namespace midismith::main_board::app::calibration {

class SensorCalibrationValidator final
    : public domain::calibration::CalibrationDataValidityRequirements {
 public:
  bool IsValidCalibration(
      const midismith::calibration::SensorCalibration& calib) const noexcept override {
    if (calib.rest_current_ma < 0.0f) return false;
    if (calib.strike_current_ma <= calib.rest_current_ma) return false;
    if (calib.strike_current_ma > config::kMaxValidStrikeCurrentMa) return false;
    return true;
  }
};

}  // namespace midismith::main_board::app::calibration

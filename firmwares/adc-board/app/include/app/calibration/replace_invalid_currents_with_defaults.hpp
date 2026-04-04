#pragma once

#include "app/config/calibration.hpp"
#include "app/config/sensors.hpp"
#include "calibration/board_calibration_data.hpp"
#include "calibration/sensor_calibration_validator.hpp"

namespace midismith::adc_board::app::calibration {

inline void ReplaceInvalidCurrentsWithDefaults(
    midismith::calibration::BoardCalibrationData<config::sensors::kSensorCount>&
        calibrations) noexcept {
  const midismith::calibration::SensorCalibrationValidator validator(
      config::kMinimumCalibrationDeltaMa, config::kMaxValidStrikeCurrentMa);
  for (auto& calib : calibrations) {
    if (!validator.IsValidCalibration(calib)) {
      calib.rest_current_ma = config::kDefaultSensorCalibration.rest_current_ma;
      calib.strike_current_ma = config::kDefaultSensorCalibration.strike_current_ma;
    }
  }
}

}  // namespace midismith::adc_board::app::calibration

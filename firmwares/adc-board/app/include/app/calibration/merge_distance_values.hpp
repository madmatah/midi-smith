#pragma once

#include "app/config/sensor_linearization.hpp"
#include "app/config/sensors.hpp"
#include "calibration/board_calibration_data.hpp"

namespace midismith::adc_board::app::calibration {

inline void MergeDistanceValues(
    midismith::calibration::BoardCalibrationData<config::sensors::kSensorCount>&
        calibrations) noexcept {
  for (auto& calib : calibrations) {
    calib.rest_distance_mm = config::kDefaultSensorCalibration.rest_distance_mm;
    calib.strike_distance_mm = config::kDefaultSensorCalibration.strike_distance_mm;
  }
}

}  // namespace midismith::adc_board::app::calibration

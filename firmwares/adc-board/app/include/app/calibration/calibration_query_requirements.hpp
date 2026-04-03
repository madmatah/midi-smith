#pragma once

#include <cstdint>

#include "calibration/sensor_calibration.hpp"

namespace midismith::adc_board::app::calibration {

class CalibrationQueryRequirements {
 public:
  virtual ~CalibrationQueryRequirements() = default;

  virtual const midismith::calibration::SensorCalibration& sensor_calibration(
      std::uint8_t sensor_index) const noexcept = 0;
};

}  // namespace midismith::adc_board::app::calibration

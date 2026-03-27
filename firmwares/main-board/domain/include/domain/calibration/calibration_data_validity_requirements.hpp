#pragma once

#include "calibration/sensor_calibration.hpp"

namespace midismith::main_board::domain::calibration {

class CalibrationDataValidityRequirements {
 public:
  virtual ~CalibrationDataValidityRequirements() = default;

  virtual bool IsValidCalibration(
      const midismith::calibration::SensorCalibration& calib) const noexcept = 0;
};

}  // namespace midismith::main_board::domain::calibration

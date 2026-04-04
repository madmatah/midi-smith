#pragma once

#include "calibration/sensor_calibration.hpp"

namespace midismith::calibration {

class CalibrationValidatorRequirements {
 public:
  virtual ~CalibrationValidatorRequirements() = default;

  virtual bool IsValidCalibration(const SensorCalibration& calib) const noexcept = 0;
};

}  // namespace midismith::calibration

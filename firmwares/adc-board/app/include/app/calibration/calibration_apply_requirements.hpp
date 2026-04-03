#pragma once

#include <cstdint>

#include "app/config/sensors.hpp"
#include "calibration/board_calibration_data.hpp"
#include "calibration/sensor_calibration.hpp"

namespace midismith::adc_board::app::calibration {

class CalibrationApplyRequirements {
 public:
  using SensorCalibrationArray = midismith::calibration::BoardCalibrationData<
      midismith::adc_board::app::config::sensors::kSensorCount>;

  virtual ~CalibrationApplyRequirements() = default;

  virtual void ApplyCalibration(const SensorCalibrationArray& data) noexcept = 0;

  virtual void ApplySensorCalibration(
      std::uint8_t sensor_index,
      const midismith::calibration::SensorCalibration& calibration) noexcept = 0;
};

}  // namespace midismith::adc_board::app::calibration

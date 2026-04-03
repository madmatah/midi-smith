#pragma once

#include <cstdint>

#include "app/config/sensors.hpp"
#include "calibration/board_calibration_data.hpp"
#include "calibration/sensor_calibration.hpp"

namespace midismith::adc_board::app::analog {

class LookupTableRegenerationRequirements {
 public:
  virtual ~LookupTableRegenerationRequirements() = default;

  virtual void RegenerateAll(
      const midismith::calibration::BoardCalibrationData<
          midismith::adc_board::app::config::sensors::kSensorCount>& data) noexcept = 0;

  virtual void RegenerateSensor(
      std::uint8_t sensor_index,
      const midismith::calibration::SensorCalibration& calibration) noexcept = 0;
};

}  // namespace midismith::adc_board::app::analog

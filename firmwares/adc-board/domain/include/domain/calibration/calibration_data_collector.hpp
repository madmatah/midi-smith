#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "calibration/board_calibration_data.hpp"
#include "calibration/calibration_validator_requirements.hpp"
#include "calibration/sensor_calibration.hpp"
#include "domain/sensors/sensor_registry.hpp"

namespace midismith::adc_board::domain::calibration {

template <std::size_t kSensorCount>
class CalibrationDataCollector {
 public:
  using CalibrationArray = midismith::calibration::BoardCalibrationData<kSensorCount>;

  explicit CalibrationDataCollector(
      const sensors::SensorRegistry& sensor_registry,
      const midismith::calibration::CalibrationValidatorRequirements& validator) noexcept
      : sensor_registry_(sensor_registry), validator_(validator) {}

  void CollectCalibrationData(CalibrationArray& calibration_data) const noexcept {
    calibration_data = CalibrationArray{};

    for (std::size_t index = 0; index < kSensorCount; ++index) {
      const auto sensor_id = static_cast<std::uint8_t>(index + 1u);
      const sensors::SensorState* const sensor_state = sensor_registry_.FindById(sensor_id);

      if (sensor_state == nullptr) {
        continue;
      }

      const midismith::calibration::SensorCalibration measured{
          .rest_current_ma = sensor_state->calibration_rest_peak_current_ma,
          .strike_current_ma = sensor_state->calibration_strike_max_current_ma,
      };

      if (validator_.IsValidCalibration(measured)) {
        calibration_data[index].rest_current_ma = measured.rest_current_ma;
        calibration_data[index].strike_current_ma = measured.strike_current_ma;
      }
    }
  }

 private:
  const sensors::SensorRegistry& sensor_registry_;
  const midismith::calibration::CalibrationValidatorRequirements& validator_;
};

}  // namespace midismith::adc_board::domain::calibration

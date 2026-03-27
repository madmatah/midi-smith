#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

#include "calibration/board_calibration_data.hpp"
#include "domain/sensors/sensor_registry.hpp"

namespace midismith::adc_board::domain::calibration {

template <std::size_t kSensorCount>
class CalibrationDataCollector {
 public:
  using CalibrationArray = midismith::calibration::BoardCalibrationData<kSensorCount>;

  explicit CalibrationDataCollector(const sensors::SensorRegistry& sensor_registry) noexcept
      : sensor_registry_(sensor_registry) {}

  CalibrationArray CollectCalibrationData() const noexcept {
    CalibrationArray result{};

    for (std::size_t index = 0; index < kSensorCount; ++index) {
      const auto sensor_id = static_cast<std::uint8_t>(index + 1u);
      const sensors::SensorState* const sensor_state = sensor_registry_.FindById(sensor_id);

      if (sensor_state == nullptr) {
        continue;
      }

      const float rest_current_ma = sensor_state->calibration_rest_peak_current_ma;
      const float strike_current_ma = sensor_state->calibration_strike_min_current_ma;

      const bool rest_is_valid = rest_current_ma > 0.0f;
      const bool strike_is_valid = strike_current_ma < std::numeric_limits<float>::max();

      if (rest_is_valid && strike_is_valid) {
        result[index].rest_current_ma = rest_current_ma;
        result[index].strike_current_ma = strike_current_ma;
      }
    }

    return result;
  }

 private:
  const sensors::SensorRegistry& sensor_registry_;
};

}  // namespace midismith::adc_board::domain::calibration

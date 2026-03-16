#pragma once

#include <limits>

#include "domain/sensors/sensor_state.hpp"

namespace midismith::adc_board::domain::sensors {

inline void ResetCalibrationState(SensorState& state) noexcept {
  state.calibration_rest_peak_current_ma = 0.0f;
  state.calibration_strike_min_current_ma = std::numeric_limits<float>::max();
  state.is_calibration_rest_phase = false;
}

}  // namespace midismith::adc_board::domain::sensors

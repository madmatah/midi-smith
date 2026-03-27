#pragma once

namespace midismith::calibration {

struct SensorCalibration {
  float rest_current_ma = 0.0f;
  float strike_current_ma = 0.0f;
  float rest_distance_mm = 0.0f;
  float strike_distance_mm = 0.0f;

  constexpr bool operator==(const SensorCalibration&) const = default;
};

}  // namespace midismith::calibration

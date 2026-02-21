#pragma once

namespace domain::sensors::linearization {

struct SensorCalibration {
  float rest_current_ma = 0.0f;
  float strike_current_ma = 0.0f;
  float rest_distance_mm = 0.0f;
  float strike_distance_mm = 0.0f;
};

}  // namespace domain::sensors::linearization

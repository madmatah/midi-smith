#pragma once

namespace midismith::sensor_linearization {

struct SensorCalibration {
  float rest_current_ma = 0.0f;
  float strike_current_ma = 0.0f;
  float rest_distance_mm = 0.0f;
  float strike_distance_mm = 0.0f;
};

}  // namespace midismith::sensor_linearization

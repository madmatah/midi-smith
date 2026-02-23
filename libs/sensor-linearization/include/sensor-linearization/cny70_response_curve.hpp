#pragma once

#include <array>

#include "sensor-linearization/sensor_response_curve.hpp"

namespace midismith::sensor_linearization {

inline constexpr std::array<SensorResponsePoint, 21> kCny70DatasheetResponseCurve = {{
    {0.0f, 1.000f}, {0.5f, 0.950f}, {1.0f, 0.850f},  {1.5f, 0.700f}, {2.0f, 0.550f}, {2.5f, 0.450f},
    {3.0f, 0.350f}, {3.5f, 0.280f}, {4.0f, 0.230f},  {4.5f, 0.190f}, {5.0f, 0.160f}, {5.5f, 0.130f},
    {6.0f, 0.110f}, {6.5f, 0.095f}, {7.0f, 0.085f},  {7.5f, 0.075f}, {8.0f, 0.065f}, {8.5f, 0.058f},
    {9.0f, 0.052f}, {9.5f, 0.048f}, {10.0f, 0.045f},
}};

inline SensorResponseCurve Cny70DatasheetSensorResponseCurve() noexcept {
  return SensorResponseCurve(kCny70DatasheetResponseCurve.data(),
                             kCny70DatasheetResponseCurve.size());
}

}  // namespace midismith::sensor_linearization

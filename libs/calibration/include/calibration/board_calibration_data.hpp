#pragma once

#include <array>
#include <cstddef>

#include "calibration/sensor_calibration.hpp"

namespace midismith::calibration {

template <std::size_t kSensorCount>
using BoardCalibrationData = std::array<SensorCalibration, kSensorCount>;

}

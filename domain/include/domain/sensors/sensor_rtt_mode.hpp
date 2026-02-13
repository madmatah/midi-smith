#pragma once

#include <cstdint>

namespace domain::sensors {

// Selects which sensor metric is sent over RTT telemetry.
enum class SensorRttMode : std::uint8_t {
  kAdc = 0,
  kAdcFiltered = 1,
  kCurrent = 2,
  kPosition = 3,
};

}  // namespace domain::sensors

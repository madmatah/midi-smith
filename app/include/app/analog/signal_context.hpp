#pragma once

#include <cstdint>

#include "domain/sensors/sensor_state.hpp"

namespace app::analog {

struct SignalContext {
  SignalContext(std::uint32_t timestamp_ticks_value, domain::sensors::SensorState& state) noexcept
      : timestamp_ticks(timestamp_ticks_value), sensor_id(state.id), sensor(state) {}

  std::uint32_t timestamp_ticks = 0;
  std::uint8_t sensor_id = 0;
  domain::sensors::SensorState& sensor;
};

}  // namespace app::analog

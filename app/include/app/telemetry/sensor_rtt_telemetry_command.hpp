#pragma once

#include <cstdint>

#include "domain/sensors/sensor_rtt_mode.hpp"

namespace app::telemetry {

enum class SensorRttTelemetryCommandKind : std::uint8_t {
  kOff = 0,
  kObserve = 1,
  kSetPeriod = 2,
};

struct SensorRttTelemetryCommand {
  SensorRttTelemetryCommandKind kind{SensorRttTelemetryCommandKind::kOff};
  std::uint8_t sensor_id{0};
  domain::sensors::SensorRttMode mode{domain::sensors::SensorRttMode::kPosition};
  std::uint32_t period_ms{0};
};

}  // namespace app::telemetry

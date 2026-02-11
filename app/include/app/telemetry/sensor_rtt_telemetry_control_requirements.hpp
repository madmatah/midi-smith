#pragma once

#include <cstdint>

#include "domain/sensors/sensor_rtt_mode.hpp"

namespace app::telemetry {

struct SensorRttTelemetryStatus {
  bool enabled{false};
  std::uint8_t sensor_id{0};
  domain::sensors::SensorRttMode mode{domain::sensors::SensorRttMode::kPosition};
  std::uint32_t period_ms{0};
};

class SensorRttTelemetryControlRequirements {
 public:
  virtual ~SensorRttTelemetryControlRequirements() = default;

  virtual bool RequestOff() noexcept = 0;
  virtual bool RequestObserve(std::uint8_t sensor_id,
                              domain::sensors::SensorRttMode mode) noexcept = 0;
  virtual bool RequestSetPeriod(std::uint32_t period_ms) noexcept = 0;
  virtual SensorRttTelemetryStatus GetStatus() const noexcept = 0;
};

}  // namespace app::telemetry

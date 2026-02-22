#pragma once

#include <cstdint>

namespace midismith::adc_board::app::telemetry {

enum class SensorRttTelemetryCommandKind : std::uint8_t {
  kOff = 0,
  kObserve = 1,
  kSetOutputHz = 2,
};

struct SensorRttTelemetryCommand {
  SensorRttTelemetryCommandKind kind{SensorRttTelemetryCommandKind::kOff};
  std::uint8_t sensor_id{0};
  std::uint32_t output_hz{0};
};

}  // namespace midismith::adc_board::app::telemetry

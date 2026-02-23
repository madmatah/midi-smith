#pragma once

#include <cstdint>
#include <string_view>

#include "app/telemetry/sensor_rtt_telemetry_control_requirements.hpp"
#include "domain/sensors/sensor_registry.hpp"
#include "domain/shell/command_requirements.hpp"

namespace midismith::adc_board::app::shell::commands {

class SensorRttCommand final : public midismith::adc_board::domain::shell::CommandRequirements {
 public:
  SensorRttCommand(
      midismith::adc_board::domain::sensors::SensorRegistry& registry,
      midismith::adc_board::app::telemetry::SensorRttTelemetryControlRequirements& control) noexcept
      : registry_(registry), control_(control) {}

  std::string_view Name() const noexcept override {
    return "sensor_rtt";
  }
  std::string_view Help() const noexcept override {
    return "Stream one sensor metrics over RTT";
  }

  void Run(int argc, char** argv, midismith::io::WritableStreamRequirements& out) noexcept override;

 private:
  midismith::adc_board::domain::sensors::SensorRegistry& registry_;
  midismith::adc_board::app::telemetry::SensorRttTelemetryControlRequirements& control_;
};

}  // namespace midismith::adc_board::app::shell::commands

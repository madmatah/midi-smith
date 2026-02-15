#pragma once

#include <cstdint>
#include <string_view>

#include "app/telemetry/sensor_rtt_telemetry_control_requirements.hpp"
#include "domain/sensors/sensor_registry.hpp"
#include "shell/command_requirements.hpp"

namespace app::shell::commands {

class SensorRttCommand final : public ::shell::CommandRequirements {
 public:
  SensorRttCommand(domain::sensors::SensorRegistry& registry,
                   app::telemetry::SensorRttTelemetryControlRequirements& control) noexcept
      : registry_(registry), control_(control) {}

  std::string_view Name() const noexcept override {
    return "sensor_rtt";
  }
  std::string_view Help() const noexcept override {
    return "Stream one sensor metrics over RTT";
  }

  void Run(int argc, char** argv, domain::io::WritableStreamRequirements& out) noexcept override;

 private:
  domain::sensors::SensorRegistry& registry_;
  app::telemetry::SensorRttTelemetryControlRequirements& control_;
};

}  // namespace app::shell::commands

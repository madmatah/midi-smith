#pragma once

#include <array>

#include "app/analog/acquisition_control_requirements.hpp"
#include "app/analog/acquisition_state_requirements.hpp"
#include "app/config/sensors.hpp"
#include "app/logging/logger_requirements.hpp"
#include "app/storage/persistent_configuration.hpp"
#include "app/telemetry/sensor_rtt_stream_capture.hpp"
#include "app/telemetry/sensor_rtt_telemetry_control_requirements.hpp"
#include "domain/io/stream_requirements.hpp"
#include "domain/sensors/linearization/sensor_calibration.hpp"
#include "domain/sensors/sensor_registry.hpp"

namespace app::composition {

struct ConfigContext {
  app::storage::PersistentConfiguration& persistent_config;
};

struct LoggingContext {
  app::Logging::LoggerRequirements& logger;
};

struct ConsoleContext {
  domain::io::StreamRequirements& stream;
};

struct AdcControlContext {
  app::analog::AcquisitionControlRequirements& control;
};

struct AdcStateContext {
  app::analog::AcquisitionStateRequirements& state;
};

struct SensorsContext {
  domain::sensors::SensorRegistry& registry;
};

struct SensorRttTelemetryControlContext {
  app::telemetry::SensorRttTelemetryControlRequirements& control;
};

ConfigContext CreateConfigSubsystem() noexcept;

AdcControlContext CreateAnalogSubsystem(app::telemetry::SensorRttStreamCapture& capture,
                                        app::Logging::LoggerRequirements& logger) noexcept;
AdcStateContext CreateAdcStateContext() noexcept;
SensorsContext CreateSensorsContext() noexcept;

bool RegenerateAnalogSensorLookupTables(
    const std::array<domain::sensors::linearization::SensorCalibration,
                     app::config_sensors::kSensorCount>& calibration_by_index) noexcept;

SensorRttTelemetryControlContext CreateSensorRttTelemetrySubsystem(
    SensorsContext& sensors, AdcStateContext& adc_state,
    app::telemetry::SensorRttStreamCapture& capture) noexcept;
void CreateShellSubsystem(ConsoleContext& console, AdcControlContext& adc_control,
                          SensorsContext& sensors,
                          SensorRttTelemetryControlContext& sensor_rtt) noexcept;

}  // namespace app::composition

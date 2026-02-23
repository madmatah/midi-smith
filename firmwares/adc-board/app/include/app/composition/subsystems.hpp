#pragma once

#include <array>

#include "app/analog/acquisition_control_requirements.hpp"
#include "app/analog/acquisition_state_requirements.hpp"
#include "app/config/sensors.hpp"
#include "app/telemetry/sensor_rtt_stream_capture.hpp"
#include "app/telemetry/sensor_rtt_telemetry_control_requirements.hpp"
#include "config/transactional_config_dictionary.hpp"
#include "domain/sensors/sensor_registry.hpp"
#include "io/stream_requirements.hpp"
#include "logging/logger_requirements.hpp"
#include "sensor-linearization/sensor_calibration.hpp"

namespace midismith::adc_board::app::composition {

struct ConfigContext {
  midismith::config::TransactionalConfigDictionary& persistent_config;
};

struct LoggingContext {
  midismith::logging::LoggerRequirements& logger;
};

struct ConsoleContext {
  midismith::io::StreamRequirements& stream;
};

struct AdcControlContext {
  midismith::adc_board::app::analog::AcquisitionControlRequirements& control;
};

struct AdcStateContext {
  midismith::adc_board::app::analog::AcquisitionStateRequirements& state;
};

struct SensorsContext {
  midismith::adc_board::domain::sensors::SensorRegistry& registry;
};

struct SensorRttTelemetryControlContext {
  midismith::adc_board::app::telemetry::SensorRttTelemetryControlRequirements& control;
};

ConfigContext CreateConfigSubsystem() noexcept;

AdcControlContext CreateAnalogSubsystem(
    midismith::adc_board::app::telemetry::SensorRttStreamCapture& capture,
    midismith::logging::LoggerRequirements& logger) noexcept;
AdcStateContext CreateAdcStateContext() noexcept;
SensorsContext CreateSensorsContext() noexcept;

bool RegenerateAnalogSensorLookupTables(
    const std::array<midismith::sensor_linearization::SensorCalibration,
                     midismith::adc_board::app::config::sensors::kSensorCount>&
        calibration_by_index) noexcept;

SensorRttTelemetryControlContext CreateSensorRttTelemetrySubsystem(
    SensorsContext& sensors, AdcStateContext& adc_state,
    midismith::adc_board::app::telemetry::SensorRttStreamCapture& capture) noexcept;
void CreateShellSubsystem(ConsoleContext& console, ConfigContext& config,
                          AdcControlContext& adc_control, SensorsContext& sensors,
                          SensorRttTelemetryControlContext& sensor_rtt) noexcept;

}  // namespace midismith::adc_board::app::composition

#pragma once

#include <array>

#include "app/analog/acquisition_control_requirements.hpp"
#include "app/analog/acquisition_state_requirements.hpp"
#include "app/config/sensors.hpp"
#include "app/logging/logger_requirements.hpp"
#include "app/telemetry/sensor_rtt_stream_capture.hpp"
#include "app/telemetry/sensor_rtt_telemetry_control_requirements.hpp"
#include "domain/config/transactional_config_dictionary.hpp"
#include "domain/io/stream_requirements.hpp"
#include "domain/sensors/linearization/sensor_calibration.hpp"
#include "domain/sensors/sensor_registry.hpp"

namespace midismith::adc_board::app::composition {

struct ConfigContext {
  midismith::adc_board::domain::config::TransactionalConfigDictionary& persistent_config;
};

struct LoggingContext {
  midismith::common::app::logging::LoggerRequirements& logger;
};

struct ConsoleContext {
  midismith::adc_board::domain::io::StreamRequirements& stream;
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
    midismith::common::app::logging::LoggerRequirements& logger) noexcept;
AdcStateContext CreateAdcStateContext() noexcept;
SensorsContext CreateSensorsContext() noexcept;

bool RegenerateAnalogSensorLookupTables(
    const std::array<midismith::adc_board::domain::sensors::linearization::SensorCalibration,
                     midismith::adc_board::app::config::sensors::kSensorCount>&
        calibration_by_index) noexcept;

SensorRttTelemetryControlContext CreateSensorRttTelemetrySubsystem(
    SensorsContext& sensors, AdcStateContext& adc_state,
    midismith::adc_board::app::telemetry::SensorRttStreamCapture& capture) noexcept;
void CreateShellSubsystem(ConsoleContext& console, ConfigContext& config,
                          AdcControlContext& adc_control, SensorsContext& sensors,
                          SensorRttTelemetryControlContext& sensor_rtt) noexcept;

}  // namespace midismith::adc_board::app::composition

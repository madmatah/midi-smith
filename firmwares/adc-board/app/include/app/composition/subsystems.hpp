#pragma once

#include <array>

#include "app/analog/acquisition_control_requirements.hpp"
#include "app/analog/acquisition_state_requirements.hpp"
#include "app/config/sensors.hpp"
#include "app/messaging/adc_board_message_sender_requirements.hpp"
#include "app/storage/adc_board_persistent_configuration.hpp"
#include "app/telemetry/sensor_rtt_stream_capture.hpp"
#include "app/telemetry/sensor_rtt_telemetry_control_requirements.hpp"
#include "bsp-types/can/can_bus_stats_requirements.hpp"
#include "bsp-types/can/fdcan_transceiver_requirements.hpp"
#include "domain/sensors/sensor_registry.hpp"
#include "io/stream_requirements.hpp"
#include "logging/logger_requirements.hpp"
#include "sensor-linearization/sensor_calibration.hpp"

namespace midismith::adc_board::app::composition {

struct CanContext {
  midismith::bsp::can::FdcanTransceiverRequirements& transceiver;
  midismith::bsp::can::CanBusStatsRequirements& stats;
};

struct ConfigContext {
  midismith::adc_board::app::storage::AdcBoardPersistentConfiguration& adc_board_config;
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

CanContext CreateCanSubsystem(midismith::logging::LoggerRequirements& logger,
                              midismith::adc_board::app::analog::AcquisitionControlRequirements&
                                  acquisition_control) noexcept;
ConfigContext CreateConfigSubsystem() noexcept;
AdcControlContext CreateAdcControlContext() noexcept;

AdcControlContext CreateAnalogSubsystem(
    midismith::adc_board::app::telemetry::SensorRttStreamCapture& capture,
    midismith::logging::LoggerRequirements& logger,
    midismith::adc_board::app::messaging::AdcBoardMessageSenderRequirements&
        message_sender) noexcept;
AdcStateContext CreateAdcStateContext() noexcept;
SensorsContext CreateSensorsContext() noexcept;

bool RegenerateAnalogSensorLookupTables(
    const std::array<midismith::sensor_linearization::SensorCalibration,
                     midismith::adc_board::app::config::sensors::kSensorCount>&
        calibration_by_index) noexcept;

SensorRttTelemetryControlContext CreateSensorRttTelemetrySubsystem(
    SensorsContext& sensors, AdcStateContext& adc_state,
    midismith::adc_board::app::telemetry::SensorRttStreamCapture& capture) noexcept;
void CreateShellSubsystem(ConsoleContext& console, CanContext& can, ConfigContext& config,
                          AdcControlContext& adc_control, SensorsContext& sensors,
                          SensorRttTelemetryControlContext& sensor_rtt) noexcept;

}  // namespace midismith::adc_board::app::composition

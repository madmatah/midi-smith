#pragma once

#include "app/analog/acquisition_control_requirements.hpp"
#include "app/analog/acquisition_state_requirements.hpp"
#include "app/analog/lookup_table_regeneration_requirements.hpp"
#include "app/calibration/calibration_apply_requirements.hpp"
#include "app/calibration/calibration_loader.hpp"
#include "app/calibration/calibration_manager.hpp"
#include "app/calibration/calibration_query_requirements.hpp"
#include "app/calibration/calibration_task.hpp"
#include "app/messaging/adc_board_message_sender_requirements.hpp"
#include "app/messaging/adc_inbound_ack_handler.hpp"
#include "app/messaging/adc_inbound_calibration_data_handler.hpp"
#include "app/storage/adc_board_persistent_configuration.hpp"
#include "app/supervisor/adc_supervisor_task.hpp"
#include "app/telemetry/sensor_rtt_stream_capture.hpp"
#include "app/telemetry/sensor_rtt_telemetry_control_requirements.hpp"
#include "bsp-types/can/can_bus_stats_requirements.hpp"
#include "bsp-types/can/fdcan_transceiver_requirements.hpp"
#include "domain/sensors/sensor_registry.hpp"
#include "io/stream_requirements.hpp"
#include "logging/logger_requirements.hpp"
#include "os-types/queue_requirements.hpp"
#include "os-types/timer_requirements.hpp"
#include "protocol-can/can_inbound_decode_stats_requirements.hpp"
#include "protocol/peer_monitor_observer_requirements.hpp"

namespace midismith::adc_board::app::composition {

struct CanContext {
  midismith::bsp::can::FdcanTransceiverRequirements& transceiver;
  midismith::bsp::can::CanBusStatsRequirements& stats;
  midismith::protocol_can::CanInboundDecodeStatsRequirements& inbound_decode_stats;
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

struct SupervisorContext {
  midismith::os::QueueRequirements<midismith::adc_board::app::supervisor::AdcSupervisorTask::Event>&
      event_queue;
};

struct CalibrationLoadInboundContext {
  midismith::adc_board::app::messaging::AdcInboundCalibrationDataHandler& handler;
};

struct CalibrationLoadContext {
  midismith::adc_board::app::calibration::CalibrationLoader& loader;
};

struct CalibrationContext {
  midismith::os::QueueRequirements<
      midismith::adc_board::app::calibration::CalibrationTask::CalibrationArray>&
      calibration_result_queue;
  midismith::os::QueueRequirements<midismith::adc_board::app::calibration::CalibrationTask::Event>&
      calibration_event_queue;
  midismith::os::TimerRequirements& rest_phase_timer;
  midismith::adc_board::app::messaging::AdcInboundAckHandler& ack_handler;
};

CalibrationContext CreateCalibrationContext(
    midismith::adc_board::app::analog::AcquisitionControlRequirements&
        acquisition_control) noexcept;
void LaunchCalibrationTask(
    midismith::adc_board::app::messaging::AdcBoardMessageSenderRequirements& sender,
    CalibrationContext& calibration_context) noexcept;

CanContext CreateCanSubsystem(
    midismith::logging::LoggerRequirements& logger,
    midismith::adc_board::app::analog::AcquisitionControlRequirements& acquisition_control,
    midismith::adc_board::app::storage::AdcBoardPersistentConfiguration& persistent_config,
    SupervisorContext& supervisor_ctx, CalibrationContext& calibration_context,
    CalibrationLoadInboundContext& calibration_load_inbound_ctx) noexcept;
ConfigContext CreateConfigSubsystem() noexcept;
AdcControlContext CreateAdcControlContext() noexcept;

AdcControlContext CreateAnalogSubsystem(
    midismith::adc_board::app::telemetry::SensorRttStreamCapture& capture,
    midismith::logging::LoggerRequirements& logger,
    midismith::adc_board::app::messaging::AdcBoardMessageSenderRequirements& message_sender,
    CalibrationContext& calibration_context) noexcept;
AdcStateContext CreateAdcStateContext() noexcept;
SensorsContext CreateSensorsContext() noexcept;

midismith::adc_board::app::analog::LookupTableRegenerationRequirements&
GetLookupTableRegenerator() noexcept;

midismith::adc_board::app::calibration::CalibrationManager& CreateCalibrationManager() noexcept;

SensorRttTelemetryControlContext CreateSensorRttTelemetrySubsystem(
    SensorsContext& sensors, AdcStateContext& adc_state,
    midismith::adc_board::app::telemetry::SensorRttStreamCapture& capture) noexcept;
SupervisorContext CreateSupervisorContext() noexcept;
void CreateSupervisorSubsystem(
    midismith::adc_board::app::messaging::AdcBoardMessageSenderRequirements& sender,
    midismith::adc_board::app::analog::AcquisitionStateRequirements& acquisition_state,
    SupervisorContext& ctx,
    midismith::protocol::PeerMonitorObserverRequirements& peer_observer) noexcept;
void CreateShellSubsystem(ConsoleContext& console, CanContext& can, ConfigContext& config,
                          AdcControlContext& adc_control, SensorsContext& sensors,
                          SensorRttTelemetryControlContext& sensor_rtt,
                          midismith::adc_board::app::calibration::CalibrationQueryRequirements&
                              calibration_query) noexcept;

CalibrationLoadInboundContext CreateCalibrationLoadInboundContext() noexcept;
CalibrationLoadContext CreateCalibrationLoadSubsystem(
    midismith::adc_board::app::messaging::AdcBoardMessageSenderRequirements& sender,
    SupervisorContext& supervisor_ctx,
    midismith::adc_board::app::calibration::CalibrationApplyRequirements&
        calibration_apply) noexcept;

}  // namespace midismith::adc_board::app::composition

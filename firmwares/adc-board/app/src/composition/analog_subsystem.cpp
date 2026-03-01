#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <utility>

#include "app/analog/queue_acquisition_control.hpp"
#include "app/analog/signal_processing/analog_sensor_processor.hpp"
#include "app/composition/subsystems.hpp"
#include "app/config/sensors.hpp"
#include "app/config/sensors_validation.hpp"
#include "app/messaging/adc_board_message_sender_requirements.hpp"
#include "app/piano_sensing/logging_sensor_event_handler.hpp"
#include "app/piano_sensing/remote_message_sensor_event_handler.hpp"
#include "app/tasks/analog_acquisition_task.hpp"
#include "app/telemetry/sensor_rtt_stream_capture.hpp"
#include "bsp/adc/adc_dma.hpp"
#include "bsp/memory_sections.hpp"
#include "bsp/pins.hpp"
#include "bsp/time/tim2_timestamp_counter.hpp"
#include "domain/sensors/processed_sensor_group.hpp"
#include "domain/sensors/sensor_registry.hpp"
#include "domain/sensors/sensor_state.hpp"
#include "os/queue.hpp"
#include "piano-sensing/composite_sensor_event_handler.hpp"
#include "sensor-linearization/lookup_table_generator.hpp"

namespace midismith::adc_board::app::composition {
namespace {

[[maybe_unused]] constexpr bool kConfigSensorsValidationIsUsed =
    midismith::adc_board::app::config::sensors::validation::AreUnique(
        midismith::adc_board::app::config::sensors::kSensorIds);

midismith::os::Queue<midismith::adc_board::app::analog::AcquisitionCommand, 4>&
AdcControlQueue() noexcept {
  static midismith::os::Queue<midismith::adc_board::app::analog::AcquisitionCommand, 4> queue;
  return queue;
}

volatile midismith::adc_board::app::analog::AcquisitionState& AdcState() noexcept {
  static volatile midismith::adc_board::app::analog::AcquisitionState state =
      midismith::adc_board::app::analog::AcquisitionState::kDisabled;
  return state;
}

midismith::adc_board::app::analog::QueueAcquisitionControl& AdcControl() noexcept {
  static midismith::adc_board::app::analog::QueueAcquisitionControl control(AdcControlQueue(),
                                                                            AdcState());
  return control;
}

std::array<midismith::adc_board::domain::sensors::SensorState,
           midismith::adc_board::app::config::sensors::kSensorCount>&
SensorsArray() noexcept {
  static std::array<midismith::adc_board::domain::sensors::SensorState,
                    midismith::adc_board::app::config::sensors::kSensorCount>
      sensors{};
  static bool sensors_initialized = false;
  if (!sensors_initialized) {
    for (std::size_t i = 0; i < midismith::adc_board::app::config::sensors::kSensorCount; ++i) {
      sensors[i].id = midismith::adc_board::app::config::sensors::kSensorIds[i];
    }
    sensors_initialized = true;
  }
  return sensors;
}

midismith::adc_board::domain::sensors::SensorRegistry& SensorsRegistry() noexcept {
  static midismith::adc_board::domain::sensors::SensorRegistry registry(
      SensorsArray().data(), midismith::adc_board::app::config::sensors::kSensorCount);
  return registry;
}

using Processor = midismith::adc_board::app::analog::signal_processing::AnalogSensorProcessor;
using ProcessedSensorGroup = midismith::adc_board::domain::sensors::ProcessedSensorGroup<
    Processor, midismith::adc_board::app::analog::SignalContext>;

using LookupTable = midismith::sensor_linearization::SensorLookupTable<
    midismith::adc_board::app::config::kSensorLookupTableSize>;
using SensorCalibration = midismith::sensor_linearization::SensorCalibration;
using LinearizerConfiguration = Processor::LinearizerConfiguration;
using LoggingSensorEventHandler =
    midismith::adc_board::app::piano_sensing::LoggingSensorEventHandler;
using RemoteMessageSensorEventHandler =
    midismith::adc_board::app::piano_sensing::RemoteMessageSensorEventHandler;
using CompositeSensorEventHandler = midismith::piano_sensing::CompositeSensorEventHandler<2>;
constexpr std::size_t kSensorCount = midismith::adc_board::app::config::sensors::kSensorCount;

std::array<LookupTable, midismith::adc_board::app::config::sensors::kSensorCount>&
LookupTablesA() noexcept {
  // Potential optimization (-23KB DTCMRAM): move to AXI SRAM by adding BSP_AXI_SRAM prefix
  static std::array<LookupTable, midismith::adc_board::app::config::sensors::kSensorCount>
      lookup_tables_a{};
  return lookup_tables_a;
}

std::array<LinearizerConfiguration, midismith::adc_board::app::config::sensors::kSensorCount>&
LinearizerConfigurationsA() noexcept {
  static std::array<LinearizerConfiguration,
                    midismith::adc_board::app::config::sensors::kSensorCount>
      configurations_a{};
  return configurations_a;
}

std::array<Processor, midismith::adc_board::app::config::sensors::kSensorCount>&
ProcessorsArray() noexcept {
  static std::array<Processor, midismith::adc_board::app::config::sensors::kSensorCount>
      processors{};
  return processors;
}

void GenerateAnalogSensorLookupTables(
    std::array<Processor, midismith::adc_board::app::config::sensors::kSensorCount>& processors,
    std::array<LookupTable, midismith::adc_board::app::config::sensors::kSensorCount>&
        lookup_tables,
    std::array<LinearizerConfiguration, midismith::adc_board::app::config::sensors::kSensorCount>&
        configurations,
    const std::array<SensorCalibration, midismith::adc_board::app::config::sensors::kSensorCount>&
        calibration_by_index) noexcept {
  const auto sensorResponseCurve =
      midismith::adc_board::app::config::kSensorResponseCurveProvider();
  for (std::size_t i = 0; i < midismith::adc_board::app::config::sensors::kSensorCount; ++i) {
    const auto result = midismith::sensor_linearization::LookupTableGenerator::Generate(
        sensorResponseCurve, calibration_by_index[i], lookup_tables[i]);

    configurations[i] = result.configuration;
    processors[i].SetLinearizerConfiguration(&configurations[i]);
  }
}

void ConfigureAnalogSensorProcessorsOnce() noexcept {
  static bool configured = false;
  if (configured) {
    return;
  }

  auto& processors = ProcessorsArray();
  auto& lookup_tables_a = LookupTablesA();
  auto& configurations_a = LinearizerConfigurationsA();
  GenerateAnalogSensorLookupTables(processors, lookup_tables_a, configurations_a,
                                   midismith::adc_board::app::config::kSensorCalibrationByIndex);

  configured = true;
}

void AttachSensorRttStreamCaptureToProcessors(
    std::array<Processor, midismith::adc_board::app::config::sensors::kSensorCount>& processors,
    midismith::adc_board::app::telemetry::SensorRttStreamCapture& capture) noexcept {
  for (std::size_t i = 0; i < midismith::adc_board::app::config::sensors::kSensorCount; ++i) {
    processors[i].SetTelemetryCapture(&capture);
  }
}

template <std::size_t... kIndex>
std::array<LoggingSensorEventHandler, kSensorCount> MakeLoggingSensorEventHandlers(
    midismith::logging::LoggerRequirements& logger, std::index_sequence<kIndex...>) noexcept {
  return {LoggingSensorEventHandler(
      logger, midismith::adc_board::app::config::sensors::kSensorIds[kIndex])...};
}

std::array<LoggingSensorEventHandler, kSensorCount>& LoggingSensorEventHandlers(
    midismith::logging::LoggerRequirements& logger) noexcept {
  static auto handlers =
      MakeLoggingSensorEventHandlers(logger, std::make_index_sequence<kSensorCount>{});
  return handlers;
}

template <std::size_t... kIndex>
std::array<RemoteMessageSensorEventHandler, kSensorCount> MakeRemoteMessageSensorEventHandlers(
    midismith::adc_board::app::messaging::AdcBoardMessageSenderRequirements& message_sender,
    std::index_sequence<kIndex...>) noexcept {
  return {RemoteMessageSensorEventHandler(
      message_sender, midismith::adc_board::app::config::sensors::kSensorIds[kIndex])...};
}

std::array<RemoteMessageSensorEventHandler, kSensorCount>& RemoteMessageSensorEventHandlers(
    midismith::adc_board::app::messaging::AdcBoardMessageSenderRequirements&
        message_sender) noexcept {
  static auto handlers = MakeRemoteMessageSensorEventHandlers(
      message_sender, std::make_index_sequence<kSensorCount>{});
  return handlers;
}

template <std::size_t... kIndex>
std::array<CompositeSensorEventHandler, kSensorCount> MakeCompositeSensorEventHandlers(
    std::array<LoggingSensorEventHandler, kSensorCount>& logging_handlers,
    std::array<RemoteMessageSensorEventHandler, kSensorCount>& remote_message_handlers,
    std::index_sequence<kIndex...>) noexcept {
  return {CompositeSensorEventHandler(
      std::array<std::reference_wrapper<midismith::piano_sensing::KeyActionRequirements>, 2>{
          std::ref(static_cast<midismith::piano_sensing::KeyActionRequirements&>(
              logging_handlers[kIndex])),
          std::ref(static_cast<midismith::piano_sensing::KeyActionRequirements&>(
              remote_message_handlers[kIndex]))})...};
}

std::array<CompositeSensorEventHandler, kSensorCount>& CompositeSensorEventHandlers(
    midismith::logging::LoggerRequirements& logger,
    midismith::adc_board::app::messaging::AdcBoardMessageSenderRequirements&
        message_sender) noexcept {
  static auto handlers = MakeCompositeSensorEventHandlers(
      LoggingSensorEventHandlers(logger), RemoteMessageSensorEventHandlers(message_sender),
      std::make_index_sequence<kSensorCount>{});
  return handlers;
}

void AttachSensorEventHandlersToProcessors(
    std::array<Processor, midismith::adc_board::app::config::sensors::kSensorCount>& processors,
    midismith::logging::LoggerRequirements& logger,
    midismith::adc_board::app::messaging::AdcBoardMessageSenderRequirements&
        message_sender) noexcept {
  auto& handlers = CompositeSensorEventHandlers(logger, message_sender);
  for (std::size_t i = 0; i < midismith::adc_board::app::config::sensors::kSensorCount; ++i) {
    processors[i].SetNoteOnKeyActionHandler(&handlers[i]);
    processors[i].SetNoteOffKeyActionHandler(&handlers[i]);
  }
}

void StartAnalogAcquisitionTask(ProcessedSensorGroup& analog_group,
                                midismith::logging::LoggerRequirements& logger) noexcept {
  static midismith::os::Queue<midismith::adc_board::bsp::adc::AdcFrameDescriptor, 8>
      adc_frame_queue;
  static midismith::adc_board::bsp::adc::AdcDma adc_dma(adc_frame_queue, logger);
  static midismith::bsp::time::TimestampCounter timestamp_counter =
      midismith::adc_board::bsp::time::CreateTim2TimestampCounter();

  alignas(midismith::adc_board::app::tasks::AnalogAcquisitionTask) static std::uint8_t
      analog_task_storage[sizeof(midismith::adc_board::app::tasks::AnalogAcquisitionTask)];
  static bool analog_constructed = false;
  midismith::adc_board::app::tasks::AnalogAcquisitionTask* analog_task_ptr = nullptr;
  if (!analog_constructed) {
    analog_task_ptr =
        new (analog_task_storage) midismith::adc_board::app::tasks::AnalogAcquisitionTask(
            adc_frame_queue, AdcControlQueue(), midismith::adc_board::bsp::pins::TiaShutdown(),
            adc_dma, timestamp_counter, AdcState(), analog_group);
    analog_constructed = true;
  } else {
    analog_task_ptr = reinterpret_cast<midismith::adc_board::app::tasks::AnalogAcquisitionTask*>(
        analog_task_storage);
  }

  (void) analog_task_ptr->start();
}

}  // namespace

AdcControlContext CreateAdcControlContext() noexcept {
  return AdcControlContext{AdcControl()};
}

AdcStateContext CreateAdcStateContext() noexcept {
  midismith::adc_board::app::analog::AcquisitionStateRequirements& state = AdcControl();
  return AdcStateContext{state};
}

SensorsContext CreateSensorsContext() noexcept {
  return SensorsContext{SensorsRegistry()};
}

bool RegenerateAnalogSensorLookupTables(
    const std::array<midismith::sensor_linearization::SensorCalibration,
                     midismith::adc_board::app::config::sensors::kSensorCount>&
        calibration_by_index) noexcept {
  if (AdcState() != midismith::adc_board::app::analog::AcquisitionState::kDisabled) {
    return false;
  }

  auto& processors = ProcessorsArray();
  auto& lookup_tables = LookupTablesA();
  auto& configurations = LinearizerConfigurationsA();
  GenerateAnalogSensorLookupTables(processors, lookup_tables, configurations, calibration_by_index);
  return true;
}

AdcControlContext CreateAnalogSubsystem(
    midismith::adc_board::app::telemetry::SensorRttStreamCapture& capture,
    midismith::logging::LoggerRequirements& logger,
    midismith::adc_board::app::messaging::AdcBoardMessageSenderRequirements&
        message_sender) noexcept {
  static_assert(midismith::adc_board::app::config::sensors::kSensorCount > 0u,
                "Sensor count must be > 0");
  static_assert(midismith::adc_board::app::config::sensors::kSensorCount == 22u,
                "Expected 22 sensors");
  static_assert(midismith::adc_board::app::config::sensors::kAdc1RankCount ==
                    midismith::adc_board::bsp::adc::AdcDma::kAdc1RanksPerSequence,
                "ADC1 rank count must match AdcDma ranks");
  static_assert(midismith::adc_board::app::config::sensors::kAdc2RankCount ==
                    midismith::adc_board::bsp::adc::AdcDma::kAdc2RanksPerSequence,
                "ADC2 rank count must match AdcDma ranks");
  static_assert(midismith::adc_board::app::config::sensors::kAdc3RankCount ==
                    midismith::adc_board::bsp::adc::AdcDma::kAdc3RanksPerSequence,
                "ADC3 rank count must match AdcDma ranks");

  ConfigureAnalogSensorProcessorsOnce();
  auto& processors = ProcessorsArray();
  AttachSensorRttStreamCaptureToProcessors(processors, capture);
  AttachSensorEventHandlersToProcessors(processors, logger, message_sender);

  auto& sensors = SensorsArray();
  static ProcessedSensorGroup analog_group(
      sensors.data(), processors.data(), midismith::adc_board::app::config::sensors::kSensorCount);

  StartAnalogAcquisitionTask(analog_group, logger);
  return AdcControlContext{AdcControl()};
}

}  // namespace midismith::adc_board::app::composition

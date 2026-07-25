#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <utility>

#include "app/analog/adc_frame.hpp"
#include "app/analog/lookup_table_regeneration_requirements.hpp"
#include "app/analog/queue_acquisition_control.hpp"
#include "app/analog/signal_processing/analog_sensor_processor.hpp"
#include "app/calibration/calibration_manager.hpp"
#include "app/composition/subsystems.hpp"
#include "app/config/calibration.hpp"
#include "app/config/calibration_validation.hpp"  // IWYU pragma: keep
#include "app/config/sensor_linearization.hpp"
#include "app/config/sensor_linearization_validation.hpp"  // IWYU pragma: keep
#include "app/config/sensors.hpp"
#include "app/config/sensors_validation.hpp"  // IWYU pragma: keep
#include "app/messaging/adc_board_message_sender_requirements.hpp"
#include "app/piano_sensing/logging_sensor_event_handler.hpp"
#include "app/piano_sensing/remote_message_sensor_event_handler.hpp"
#include "app/tasks/analog_acquisition_task.hpp"
#include "app/telemetry/sensor_rtt_stream_capture.hpp"
#include "bsp/adc/adc_dma.hpp"
#include "bsp/memory_sections.hpp"
#include "bsp/pins.hpp"
#include "bsp/time/tim2_timestamp_counter.hpp"
#include "calibration/board_calibration_data.hpp"
#include "calibration/sensor_calibration_validator.hpp"
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
using SensorCalibration = midismith::calibration::SensorCalibration;
using LinearizerConfiguration = Processor::LinearizerConfiguration;
using LoggingSensorEventHandler =
    midismith::adc_board::app::piano_sensing::LoggingSensorEventHandler;
using RemoteMessageSensorEventHandler =
    midismith::adc_board::app::piano_sensing::RemoteMessageSensorEventHandler;
using CompositeSensorEventHandler = midismith::piano_sensing::CompositeSensorEventHandler<2>;
constexpr std::size_t kSensorCount = midismith::adc_board::app::config::sensors::kSensorCount;

std::array<LookupTable, midismith::adc_board::app::config::sensors::kSensorCount>&
LookupTablesA() noexcept {
  BSP_AXI_SRAM static std::array<LookupTable,
                                 midismith::adc_board::app::config::sensors::kSensorCount>
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

std::array<LookupTable, midismith::adc_board::app::config::sensors::kSensorCount>&
LookupTablesB() noexcept {
  BSP_AXI_SRAM static std::array<LookupTable,
                                 midismith::adc_board::app::config::sensors::kSensorCount>
      lookup_tables_b{};
  return lookup_tables_b;
}

std::array<LinearizerConfiguration, midismith::adc_board::app::config::sensors::kSensorCount>&
LinearizerConfigurationsB() noexcept {
  static std::array<LinearizerConfiguration,
                    midismith::adc_board::app::config::sensors::kSensorCount>
      configurations_b{};
  return configurations_b;
}

std::array<Processor, midismith::adc_board::app::config::sensors::kSensorCount>&
ProcessorsArray() noexcept {
  static std::array<Processor, midismith::adc_board::app::config::sensors::kSensorCount>
      processors{};
  return processors;
}

class AnalogLookupTableRegenerator final
    : public midismith::adc_board::app::analog::LookupTableRegenerationRequirements {
 public:
  void RegenerateSensor(
      std::uint8_t sensor_index,
      const midismith::calibration::SensorCalibration& calibration) noexcept override {
    const bool write_to_a = !buffer_a_is_active_[sensor_index];

    LookupTable& target_lut =
        write_to_a ? LookupTablesA()[sensor_index] : LookupTablesB()[sensor_index];
    LinearizerConfiguration& target_config = write_to_a ? LinearizerConfigurationsA()[sensor_index]
                                                        : LinearizerConfigurationsB()[sensor_index];

    const auto curve = midismith::adc_board::app::config::kSensorResponseCurveProvider();
    const auto result = midismith::sensor_linearization::LookupTableGenerator::Generate(
        curve, calibration, target_lut);
    target_config = result.configuration;
    ProcessorsArray()[sensor_index].SetLinearizerConfiguration(&target_config);
    buffer_a_is_active_[sensor_index] = write_to_a;
  }

  void RegenerateAll(
      const midismith::calibration::BoardCalibrationData<
          midismith::adc_board::app::config::sensors::kSensorCount>& data) noexcept override {
    for (std::uint8_t i = 0; i < midismith::adc_board::app::config::sensors::kSensorCount; ++i) {
      RegenerateSensor(i, data[i]);
    }
  }

 private:
  bool buffer_a_is_active_[midismith::adc_board::app::config::sensors::kSensorCount]{};
};

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

void StartAnalogAcquisitionTask(
    ProcessedSensorGroup& analog_group, midismith::logging::LoggerRequirements& logger,
    midismith::os::QueueRequirements<
        midismith::adc_board::app::tasks::AnalogAcquisitionTask::CalibrationArray>&
        calibration_result_queue) noexcept {
  static midismith::os::Queue<midismith::adc_board::app::analog::AdcFrameDescriptor, 8>
      adc_frame_queue;
  static midismith::adc_board::bsp::adc::AdcDma adc_dma(adc_frame_queue, logger);
  static midismith::bsp::time::TimestampCounter timestamp_counter =
      midismith::adc_board::bsp::time::CreateTim2TimestampCounter();

  alignas(midismith::adc_board::app::tasks::AnalogAcquisitionTask) static std::uint8_t
      analog_task_storage[sizeof(midismith::adc_board::app::tasks::AnalogAcquisitionTask)];
  static midismith::calibration::SensorCalibrationValidator calibration_validator(
      midismith::adc_board::app::config::kMinimumCalibrationDeltaMa,
      midismith::adc_board::app::config::kMaxValidStrikeCurrentMa);

  static bool analog_constructed = false;
  midismith::adc_board::app::tasks::AnalogAcquisitionTask* analog_task_ptr = nullptr;
  if (!analog_constructed) {
    analog_task_ptr =
        new (analog_task_storage) midismith::adc_board::app::tasks::AnalogAcquisitionTask(
            adc_frame_queue, AdcControlQueue(), midismith::adc_board::bsp::pins::TiaShutdown(),
            adc_dma, timestamp_counter, AdcState(), analog_group, calibration_result_queue,
            SensorsRegistry(), calibration_validator);
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

midismith::adc_board::app::analog::LookupTableRegenerationRequirements&
GetLookupTableRegenerator() noexcept {
  static AnalogLookupTableRegenerator regenerator;
  return regenerator;
}

midismith::adc_board::app::calibration::CalibrationManager& CreateCalibrationManager() noexcept {
  static midismith::adc_board::app::calibration::CalibrationManager manager(
      GetLookupTableRegenerator());
  return manager;
}

AdcControlContext CreateAnalogSubsystem(
    midismith::adc_board::app::telemetry::SensorRttStreamCapture& capture,
    midismith::logging::LoggerRequirements& logger,
    midismith::adc_board::app::messaging::AdcBoardMessageSenderRequirements& message_sender,
    CalibrationContext& calibration_context) noexcept {
  static_assert(midismith::adc_board::app::config::sensors::kSensorCount > 0u,
                "Sensor count must be > 0");
  static_assert(midismith::adc_board::app::config::sensors::kSensorCount == 22u,
                "Expected 22 sensors");

  auto& processors = ProcessorsArray();
  AttachSensorRttStreamCaptureToProcessors(processors, capture);
  AttachSensorEventHandlersToProcessors(processors, logger, message_sender);

  auto& sensors = SensorsArray();
  static ProcessedSensorGroup analog_group(
      sensors.data(), processors.data(), midismith::adc_board::app::config::sensors::kSensorCount);

  StartAnalogAcquisitionTask(analog_group, logger, calibration_context.calibration_result_queue);
  return AdcControlContext{AdcControl()};
}

}  // namespace midismith::adc_board::app::composition

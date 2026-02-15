#include <array>
#include <cstddef>
#include <cstdint>

#include "app/analog/queue_acquisition_control.hpp"
#include "app/analog/sensors/logging_sensor_event_handler.hpp"
#include "app/composition/subsystems.hpp"
#include "app/config/sensor_linearization_validation.hpp"
#include "app/config/sensors.hpp"
#include "app/config/sensors_validation.hpp"
#include "app/tasks/analog_acquisition_task.hpp"
#include "app/telemetry/sensor_rtt_stream_capture.hpp"
#include "bsp/adc/adc_dma.hpp"
#include "bsp/pins.hpp"
#include "bsp/time/tim2_timestamp_counter.hpp"
#include "domain/sensors/linearization/lookup_table_generator.hpp"
#include "domain/sensors/processed_sensor_group.hpp"
#include "domain/sensors/sensor_registry.hpp"
#include "domain/sensors/sensor_state.hpp"
#include "os/queue.hpp"

namespace app::composition {
namespace {

[[maybe_unused]] constexpr bool kConfigSensorsValidationIsUsed =
    app::config_sensors::validation::AreUnique(app::config_sensors::kSensorIds);

os::Queue<app::analog::AcquisitionCommand, 4>& AdcControlQueue() noexcept {
  static os::Queue<app::analog::AcquisitionCommand, 4> queue;
  return queue;
}

volatile app::analog::AcquisitionState& AdcState() noexcept {
  static volatile app::analog::AcquisitionState state = app::analog::AcquisitionState::kDisabled;
  return state;
}

app::analog::QueueAcquisitionControl& AdcControl() noexcept {
  static app::analog::QueueAcquisitionControl control(AdcControlQueue(), AdcState());
  return control;
}

std::array<domain::sensors::SensorState, app::config_sensors::kSensorCount>&
SensorsArray() noexcept {
  static std::array<domain::sensors::SensorState, app::config_sensors::kSensorCount> sensors{};
  static bool sensors_initialized = false;
  if (!sensors_initialized) {
    for (std::size_t i = 0; i < app::config_sensors::kSensorCount; ++i) {
      sensors[i].id = app::config_sensors::kSensorIds[i];
    }
    sensors_initialized = true;
  }
  return sensors;
}

domain::sensors::SensorRegistry& SensorsRegistry() noexcept {
  static domain::sensors::SensorRegistry registry(SensorsArray().data(),
                                                  app::config_sensors::kSensorCount);
  return registry;
}

using Processor = app::config::AnalogSensorProcessor;
using ProcessedSensorGroup =
    domain::sensors::ProcessedSensorGroup<Processor, app::analog::SignalContext>;

using LookupTable =
    domain::sensors::linearization::SensorLookupTable<app::config::kSensorLookupTableSize>;
using SensorCalibration = domain::sensors::linearization::SensorCalibration;
using LinearizerConfiguration = domain::sensors::linearization::SensorLinearProcessorConfiguration<
    app::config::kSensorLookupTableSize>;

std::array<LookupTable, app::config_sensors::kSensorCount>& LookupTablesA() noexcept {
  static std::array<LookupTable, app::config_sensors::kSensorCount> lookup_tables_a{};
  return lookup_tables_a;
}

std::array<LinearizerConfiguration, app::config_sensors::kSensorCount>&
LinearizerConfigurationsA() noexcept {
  static std::array<LinearizerConfiguration, app::config_sensors::kSensorCount> configurations_a{};
  return configurations_a;
}

std::array<Processor, app::config_sensors::kSensorCount>& ProcessorsArray() noexcept {
  static std::array<Processor, app::config_sensors::kSensorCount> processors{};
  return processors;
}

void GenerateAnalogSensorLookupTables(
    std::array<Processor, app::config_sensors::kSensorCount>& processors,
    std::array<LookupTable, app::config_sensors::kSensorCount>& lookup_tables,
    std::array<LinearizerConfiguration, app::config_sensors::kSensorCount>& configurations,
    const std::array<SensorCalibration, app::config_sensors::kSensorCount>&
        calibration_by_index) noexcept {
  const auto sensorResponseCurve = app::config::kSensorResponseCurveProvider();
  for (std::size_t i = 0; i < app::config_sensors::kSensorCount; ++i) {
    const auto result = domain::sensors::linearization::LookupTableGenerator::Generate(
        sensorResponseCurve, calibration_by_index[i], lookup_tables[i]);

    configurations[i] = result.configuration;

    auto& linearizer =
        processors[i].Stage<app::config::kAnalogSensorProcessorLinearizerStageIndex>();
    linearizer.ApplyConfiguration(&configurations[i]);
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
                                   app::config::kSensorCalibrationByIndex);

  configured = true;
}

void AttachSensorRttStreamCaptureToProcessors(
    std::array<Processor, app::config_sensors::kSensorCount>& processors,
    app::telemetry::SensorRttStreamCapture& capture) noexcept {
  for (std::size_t i = 0; i < app::config_sensors::kSensorCount; ++i) {
    auto& tap = processors[i].Stage<app::config::kAnalogSensorProcessorRttTapStageIndex>();
    tap.SetCapture(&capture);
  }
}

std::array<app::analog::sensors::LoggingSensorEventHandler, app::config_sensors::kSensorCount>&
VelocityHandlers() noexcept {
  static std::array<app::analog::sensors::LoggingSensorEventHandler,
                    app::config_sensors::kSensorCount>
      handlers{};
  return handlers;
}

void AttachSensorVelocityHandlersToProcessors(
    std::array<Processor, app::config_sensors::kSensorCount>& processors,
    app::Logging::LoggerRequirements& logger) noexcept {
  auto& handlers = VelocityHandlers();
  for (std::size_t i = 0; i < app::config_sensors::kSensorCount; ++i) {
    handlers[i].SetLogger(&logger);
    handlers[i].SetSensorId(app::config_sensors::kSensorIds[i]);

    auto& tap = processors[i].Stage<app::config::kAnalogSensorProcessorMidiVelocityTapStageIndex>();
    tap.Content().SetKeyActionHandler(&handlers[i]);

    auto& release_tap =
        processors[i].Stage<app::config::kAnalogSensorProcessorNoteReleaseTapStageIndex>();
    release_tap.Content().SetKeyActionHandler(&handlers[i]);
  }
}

void StartAnalogAcquisitionTask(ProcessedSensorGroup& analog_group) noexcept {
  static os::Queue<bsp::adc::AdcFrameDescriptor, 8> adc_frame_queue;
  static bsp::adc::AdcDma adc_dma(adc_frame_queue);
  static bsp::time::TimestampCounter timestamp_counter = bsp::time::CreateTim2TimestampCounter();

  alignas(app::Tasks::AnalogAcquisitionTask) static std::uint8_t
      analog_task_storage[sizeof(app::Tasks::AnalogAcquisitionTask)];
  static bool analog_constructed = false;
  app::Tasks::AnalogAcquisitionTask* analog_task_ptr = nullptr;
  if (!analog_constructed) {
    analog_task_ptr = new (analog_task_storage) app::Tasks::AnalogAcquisitionTask(
        adc_frame_queue, AdcControlQueue(), bsp::pins::TiaShutdown(), adc_dma, timestamp_counter,
        AdcState(), analog_group);
    analog_constructed = true;
  } else {
    analog_task_ptr = reinterpret_cast<app::Tasks::AnalogAcquisitionTask*>(analog_task_storage);
  }

  (void) analog_task_ptr->start();
}

}  // namespace

AdcStateContext CreateAdcStateContext() noexcept {
  app::analog::AcquisitionStateRequirements& state = AdcControl();
  return AdcStateContext{state};
}

SensorsContext CreateSensorsContext() noexcept {
  return SensorsContext{SensorsRegistry()};
}

bool RegenerateAnalogSensorLookupTables(
    const std::array<domain::sensors::linearization::SensorCalibration,
                     app::config_sensors::kSensorCount>& calibration_by_index) noexcept {
  if (AdcState() != app::analog::AcquisitionState::kDisabled) {
    return false;
  }

  auto& processors = ProcessorsArray();
  auto& lookup_tables = LookupTablesA();
  auto& configurations = LinearizerConfigurationsA();
  GenerateAnalogSensorLookupTables(processors, lookup_tables, configurations, calibration_by_index);
  return true;
}

AdcControlContext CreateAnalogSubsystem(app::telemetry::SensorRttStreamCapture& capture,
                                        app::Logging::LoggerRequirements& logger) noexcept {
  static_assert(app::config_sensors::kSensorCount > 0u, "Sensor count must be > 0");
  static_assert(app::config_sensors::kSensorCount == 22u, "Expected 22 sensors");
  static_assert(app::config_sensors::kAdc1RankCount == bsp::adc::AdcDma::kAdc1RanksPerSequence,
                "ADC1 rank count must match AdcDma ranks");
  static_assert(app::config_sensors::kAdc2RankCount == bsp::adc::AdcDma::kAdc2RanksPerSequence,
                "ADC2 rank count must match AdcDma ranks");
  static_assert(app::config_sensors::kAdc3RankCount == bsp::adc::AdcDma::kAdc3RanksPerSequence,
                "ADC3 rank count must match AdcDma ranks");

  ConfigureAnalogSensorProcessorsOnce();
  auto& processors = ProcessorsArray();
  AttachSensorRttStreamCaptureToProcessors(processors, capture);
  AttachSensorVelocityHandlersToProcessors(processors, logger);

  auto& sensors = SensorsArray();
  static ProcessedSensorGroup analog_group(sensors.data(), processors.data(),
                                           app::config_sensors::kSensorCount);

  StartAnalogAcquisitionTask(analog_group);
  return AdcControlContext{AdcControl()};
}

}  // namespace app::composition

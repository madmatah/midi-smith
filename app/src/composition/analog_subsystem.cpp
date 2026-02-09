#include <array>
#include <cstddef>
#include <cstdint>

#include "app/analog/queue_acquisition_control.hpp"
#include "app/composition/subsystems.hpp"
#include "app/config/sensors.hpp"
#include "app/config/sensors_validation.hpp"
#include "app/tasks/analog_acquisition_task.hpp"
#include "bsp/adc/adc_dma.hpp"
#include "bsp/pins.hpp"
#include "bsp/time/tim2_timestamp_counter.hpp"
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

AdcControlContext CreateAnalogSubsystem() noexcept {
  static_assert(app::config_sensors::kSensorCount > 0u, "Sensor count must be > 0");
  static_assert(app::config_sensors::kSensorCount == 22u, "Expected 22 sensors");
  static_assert(app::config_sensors::kAdc1RankCount == bsp::adc::AdcDma::kAdc1RanksPerSequence,
                "ADC1 rank count must match AdcDma ranks");
  static_assert(app::config_sensors::kAdc2RankCount == bsp::adc::AdcDma::kAdc2RanksPerSequence,
                "ADC2 rank count must match AdcDma ranks");
  static_assert(app::config_sensors::kAdc3RankCount == bsp::adc::AdcDma::kAdc3RanksPerSequence,
                "ADC3 rank count must match AdcDma ranks");

  static std::array<Processor, app::config_sensors::kSensorCount> processors{};

  auto& sensors = SensorsArray();
  static ProcessedSensorGroup analog_group(sensors.data(), processors.data(),
                                           app::config_sensors::kSensorCount);

  StartAnalogAcquisitionTask(analog_group);
  return AdcControlContext{AdcControl()};
}

}  // namespace app::composition

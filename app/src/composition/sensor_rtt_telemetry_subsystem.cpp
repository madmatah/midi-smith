#include <cstdint>
#include <new>

#include "app/composition/subsystems.hpp"
#include "app/config/config.hpp"
#include "app/tasks/sensor_rtt_telemetry_task.hpp"
#include "app/telemetry/queue_sensor_rtt_telemetry_control.hpp"
#include "bsp/rtt_telemetry_sender.hpp"
#include "os/queue.hpp"

namespace app::composition {

SensorRttTelemetryControlContext CreateSensorRttTelemetrySubsystem(
    SensorsContext& sensors, AdcStateContext& adc_state) noexcept {
  alignas(4) static std::uint8_t telemetry_buffer[app::config::RTT_TELEMETRY_SENSOR_BUFFER_SIZE];
  static bsp::RttTelemetrySender telemetry(app::config::RTT_TELEMETRY_SENSOR_CHANNEL,
                                           "SensorTelemetry", telemetry_buffer,
                                           static_cast<unsigned>(sizeof(telemetry_buffer)));

  static os::Queue<app::telemetry::SensorRttTelemetryCommand, 4> control_queue;
  static volatile bool enabled = false;
  static volatile std::uint8_t sensor_id = 0;
  static volatile domain::sensors::SensorRttMode mode = domain::sensors::SensorRttMode::kPosition;
  static volatile std::uint32_t period_ms = app::config::RTT_TELEMETRY_SENSOR_PERIOD_MS;
  static app::telemetry::QueueSensorRttTelemetryControl control(control_queue, enabled, sensor_id,
                                                                mode, period_ms);

  alignas(app::Tasks::SensorRttTelemetryTask) static std::uint8_t
      task_storage[sizeof(app::Tasks::SensorRttTelemetryTask)];
  static bool task_constructed = false;
  app::Tasks::SensorRttTelemetryTask* task_ptr = nullptr;
  if (!task_constructed) {
    task_ptr = new (task_storage)
        app::Tasks::SensorRttTelemetryTask(control_queue, sensors.registry, adc_state.state,
                                           telemetry, enabled, sensor_id, mode, period_ms);
    task_constructed = true;
  } else {
    task_ptr = reinterpret_cast<app::Tasks::SensorRttTelemetryTask*>(task_storage);
  }

  (void) task_ptr->start();
  return SensorRttTelemetryControlContext{control};
}

}  // namespace app::composition

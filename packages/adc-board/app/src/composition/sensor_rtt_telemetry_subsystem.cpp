#include <cstdint>
#include <new>

#include "app/composition/subsystems.hpp"
#include "app/config/config.hpp"
#include "app/tasks/sensor_rtt_telemetry_task.hpp"
#include "app/telemetry/queue_sensor_rtt_telemetry_control.hpp"
#include "bsp/memory_sections.hpp"
#include "bsp/rtt_telemetry_sender.hpp"
#include "os/queue.hpp"

namespace app::composition {

SensorRttTelemetryControlContext CreateSensorRttTelemetrySubsystem(
    SensorsContext& sensors, AdcStateContext& adc_state,
    app::telemetry::SensorRttStreamCapture& capture) noexcept {
  alignas(4) BSP_AXI_SRAM static std::uint8_t
      telemetry_buffer[app::config::RTT_TELEMETRY_SENSOR_BUFFER_SIZE];
  static bsp::RttTelemetrySender telemetry(app::config::RTT_TELEMETRY_SENSOR_CHANNEL,
                                           "SensorTelemetry", telemetry_buffer,
                                           static_cast<unsigned>(sizeof(telemetry_buffer)));

  static os::Queue<app::telemetry::SensorRttTelemetryCommand, 4> control_queue;
  static app::telemetry::QueueSensorRttTelemetryControl control(control_queue, capture);

  alignas(app::Tasks::SensorRttTelemetryTask) static std::uint8_t
      task_storage[sizeof(app::Tasks::SensorRttTelemetryTask)];
  static bool task_constructed = false;
  app::Tasks::SensorRttTelemetryTask* task_ptr = nullptr;
  if (!task_constructed) {
    task_ptr = new (task_storage) app::Tasks::SensorRttTelemetryTask(
        control_queue, sensors.registry, adc_state.state, telemetry, capture);
    task_constructed = true;
  } else {
    task_ptr = reinterpret_cast<app::Tasks::SensorRttTelemetryTask*>(task_storage);
  }

  (void) task_ptr->start();
  return SensorRttTelemetryControlContext{control};
}

}  // namespace app::composition

#include <cstdint>
#include <new>

#include "app/composition/subsystems.hpp"
#include "app/config/config.hpp"
#include "app/tasks/sensor_rtt_telemetry_task.hpp"
#include "app/telemetry/queue_sensor_rtt_telemetry_control.hpp"
#include "app/telemetry/telemetry_sender_requirements.hpp"
#include "bsp/memory_sections.hpp"
#include "bsp/rtt_telemetry_sender.hpp"
#include "os/queue.hpp"

namespace midismith::adc_board::app::composition {
namespace {

class RttTelemetrySenderAdapter final
    : public midismith::adc_board::app::telemetry::TelemetrySenderRequirements {
 public:
  explicit RttTelemetrySenderAdapter(midismith::bsp::RttTelemetrySender& sender) noexcept
      : sender_(sender) {}

  std::size_t Send(std::span<const std::uint8_t> data) noexcept override {
    return sender_.Send(data);
  }

 private:
  midismith::bsp::RttTelemetrySender& sender_;
};

}  // namespace

SensorRttTelemetryControlContext CreateSensorRttTelemetrySubsystem(
    SensorsContext& sensors, AdcStateContext& adc_state,
    midismith::adc_board::app::telemetry::SensorRttStreamCapture& capture) noexcept {
  alignas(4) BSP_AXI_SRAM static std::uint8_t
      telemetry_buffer[midismith::adc_board::app::config::RTT_TELEMETRY_SENSOR_BUFFER_SIZE];
  static midismith::bsp::RttTelemetrySender telemetry_sender(
      midismith::adc_board::app::config::RTT_TELEMETRY_SENSOR_CHANNEL, "SensorTelemetry",
      telemetry_buffer, static_cast<unsigned>(sizeof(telemetry_buffer)));
  static RttTelemetrySenderAdapter telemetry(telemetry_sender);

  static midismith::os::Queue<midismith::adc_board::app::telemetry::SensorRttTelemetryCommand, 4>
      control_queue;
  static midismith::adc_board::app::telemetry::QueueSensorRttTelemetryControl control(control_queue,
                                                                                      capture);

  alignas(midismith::adc_board::app::tasks::SensorRttTelemetryTask) static std::uint8_t
      task_storage[sizeof(midismith::adc_board::app::tasks::SensorRttTelemetryTask)];
  static bool task_constructed = false;
  midismith::adc_board::app::tasks::SensorRttTelemetryTask* task_ptr = nullptr;
  if (!task_constructed) {
    task_ptr = new (task_storage) midismith::adc_board::app::tasks::SensorRttTelemetryTask(
        control_queue, sensors.registry, adc_state.state, telemetry, capture);
    task_constructed = true;
  } else {
    task_ptr =
        reinterpret_cast<midismith::adc_board::app::tasks::SensorRttTelemetryTask*>(task_storage);
  }

  (void) task_ptr->start();
  return SensorRttTelemetryControlContext{control};
}

}  // namespace midismith::adc_board::app::composition

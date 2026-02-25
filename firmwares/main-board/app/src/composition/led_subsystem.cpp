#include <new>
#include <span>

#include "app/composition/subsystems.hpp"
#include "app/config.hpp"
#include "app/tasks/led_task.hpp"
#include "bsp/board.hpp"
#include "bsp/rtt_telemetry_sender.hpp"
#include "telemetry/telemetry_sender_requirements.hpp"

namespace midismith::main_board::app::composition {

namespace {

class RttTelemetrySenderAdapter final : public midismith::telemetry::TelemetrySenderRequirements {
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

void CreateLedSubsystem(MidiContext& midi) noexcept {
  alignas(midismith::main_board::app::tasks::LedTask) static std::uint8_t
      led_task_storage[sizeof(midismith::main_board::app::tasks::LedTask)];
  static bool led_constructed = false;

  auto& led = midismith::main_board::bsp::Board::user_led();

  alignas(4) static std::uint8_t
      led_telemetry_buffer[midismith::main_board::app::config::RTT_TELEMETRY_LED_BUFFER_SIZE];
  static midismith::bsp::RttTelemetrySender telemetry_sender(
      midismith::main_board::app::config::RTT_TELEMETRY_LED_CHANNEL, "LedTelemetry",
      led_telemetry_buffer, sizeof(led_telemetry_buffer));
  static RttTelemetrySenderAdapter telemetry(telemetry_sender);

  midismith::main_board::app::tasks::LedTask* led_task_ptr = nullptr;
  if (!led_constructed) {
    led_task_ptr = new (led_task_storage)
        midismith::main_board::app::tasks::LedTask(led, telemetry, midi.piano);
    led_constructed = true;
  } else {
    led_task_ptr = reinterpret_cast<midismith::main_board::app::tasks::LedTask*>(led_task_storage);
  }

  (void) led_task_ptr->start();
}

}  // namespace midismith::main_board::app::composition

#include "app/tasks/led_task.hpp"

#include "app/config.hpp"
#include "app/telemetry/telemetry_sender_requirements.hpp"
#include "bsp/gpio_requirements.hpp"
#include "domain/music/piano/piano_requirements.hpp"
#include "domain/music/types.hpp"
#include "os/clock.hpp"
#include "os/task.hpp"

namespace app::Tasks {

LedTask::LedTask(bsp::GpioRequirements& led, app::telemetry::TelemetrySenderRequirements& telemetry,
                 domain::music::piano::PianoRequirements& piano) noexcept
    : _led(led), _telemetry(telemetry), _piano(piano) {}

void LedTask::entry(void* ctx) noexcept {
  if (ctx == nullptr) {
    return;
  }
  static_cast<LedTask*>(ctx)->run();
}

void LedTask::run() noexcept {
  for (;;) {
    _led.toggle();
    const bool state = _led.read();

    if (state) {
      _piano.PressKey(domain::music::kNoteC4, 60);
    } else {
      _piano.ReleaseKey(domain::music::kNoteC4);
    }

    _telemetry.Send(state ? 1u : 0u);
    os::Clock::delay_ms(app::config::LED_BLINK_PERIOD_MS);
  }
}

bool LedTask::start() noexcept {
  return os::Task::create("LedTask", LedTask::entry, this, app::config::LED_TASK_STACK_BYTES,
                          app::config::LED_TASK_PRIORITY);
}

}  // namespace app::Tasks

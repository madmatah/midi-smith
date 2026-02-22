#include "app/tasks/led_task.hpp"

#include "app/config.hpp"
#include "app/telemetry/telemetry_sender_requirements.hpp"
#include "bsp/gpio_requirements.hpp"
#include "domain/music/piano/piano_requirements.hpp"
#include "domain/music/types.hpp"
#include "os/clock.hpp"
#include "os/task.hpp"

namespace midismith::main_board::app::tasks {

LedTask::LedTask(midismith::common::bsp::GpioRequirements& led,
                 midismith::main_board::app::telemetry::TelemetrySenderRequirements& telemetry,
                 midismith::main_board::domain::music::piano::PianoRequirements& piano) noexcept
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
      _piano.PressKey(midismith::common::domain::music::kNoteC4, 60);
    } else {
      _piano.ReleaseKey(midismith::common::domain::music::kNoteC4);
    }

    _telemetry.Send(state ? 1u : 0u);
    midismith::common::os::Clock::delay_ms(midismith::main_board::app::config::LED_BLINK_PERIOD_MS);
  }
}

bool LedTask::start() noexcept {
  return midismith::common::os::Task::create(
      "LedTask", LedTask::entry, this, midismith::main_board::app::config::LED_TASK_STACK_BYTES,
      midismith::main_board::app::config::LED_TASK_PRIORITY);
}

}  // namespace midismith::main_board::app::tasks

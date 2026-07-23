#include "app/ui/ui_task.hpp"

#include "app/config/ui.hpp"
#include "os/clock.hpp"
#include "os/task.hpp"

namespace midismith::main_board::app::ui {

UiTask::UiTask(midismith::main_board::bsp::RotaryEncoder& encoder,
               midismith::main_board::bsp::RotaryButton& button,
               midismith::menu::MenuRuntime& runtime,
               midismith::text_display::TextDisplayRequirements& display,
               std::uint32_t tick_period_ms, InitializeCallback initialize_callback,
               void* initialize_context) noexcept
    : encoder_(encoder),
      button_(button),
      runtime_(runtime),
      display_(display),
      tick_period_ms_(tick_period_ms),
      initialize_callback_(initialize_callback),
      initialize_context_(initialize_context) {}

void UiTask::entry(void* context) noexcept {
  if (context == nullptr) {
    return;
  }
  static_cast<UiTask*>(context)->run();
}

void UiTask::run() noexcept {
  if (initialize_callback_ != nullptr) {
    initialize_callback_(initialize_context_);
  }
  encoder_.Start();
  runtime_.Render(display_);
  display_.Flush();
  for (;;) {
    midismith::os::Clock::delay_ms(tick_period_ms_);
    DispatchRotation(encoder_.ReadDeltaDetents());
    DispatchButton(button_.Poll());
    if (runtime_.is_dirty()) {
      runtime_.Render(display_);
      display_.Flush();
    }
  }
}

bool UiTask::start() noexcept {
  return midismith::os::Task::create("UiTask", UiTask::entry, this,
                                     midismith::main_board::app::config::kUiTaskStackBytes,
                                     midismith::main_board::app::config::kUiTaskPriority);
}

void UiTask::DispatchRotation(std::int16_t detents) noexcept {
  while (detents > 0) {
    runtime_.HandleInput(midismith::menu::InputEvent::Rotate(1));
    detents--;
  }
  while (detents < 0) {
    runtime_.HandleInput(midismith::menu::InputEvent::Rotate(-1));
    detents++;
  }
}

void UiTask::DispatchButton(midismith::main_board::bsp::RotaryButton::Event event) noexcept {
  if (event == midismith::main_board::bsp::RotaryButton::Event::kPressed) {
    runtime_.HandleInput(midismith::menu::InputEvent::ButtonPress());
  } else if (event == midismith::main_board::bsp::RotaryButton::Event::kLongPressed) {
    runtime_.HandleInput(midismith::menu::InputEvent::ButtonLongPress());
  }
}

}  // namespace midismith::main_board::app::ui

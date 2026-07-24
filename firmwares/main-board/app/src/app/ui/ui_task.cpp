#include "app/ui/ui_task.hpp"

#include <array>
#include <charconv>
#include <string_view>

#include "app/config/ui.hpp"
#include "os/clock.hpp"
#include "os/task.hpp"

namespace midismith::main_board::app::ui {

UiTask::UiTask(midismith::main_board::bsp::RotaryEncoder& encoder,
               midismith::main_board::bsp::RotaryButton& button,
               midismith::menu::MenuRuntime& runtime,
               midismith::text_display::TextDisplayRequirements& display,
               DisplayPowerRequirements& display_power, SplashRequirements& splash,
               midismith::os::QueueRequirements<midismith::menu::InputEvent>& injected_events,
               ActivitySourceRequirements& wake_activity, std::uint32_t tick_period_ms,
               InitializeCallback initialize_callback, void* initialize_context) noexcept
    : encoder_(encoder),
      button_(button),
      runtime_(runtime),
      display_(display),
      display_power_(display_power),
      splash_(splash),
      injected_events_(injected_events),
      wake_activity_(wake_activity),
      tick_period_ms_(tick_period_ms),
      initialize_callback_(initialize_callback),
      initialize_context_(initialize_context),
      idle_tracker_(midismith::main_board::app::config::kUiBacklightTimeoutMs / tick_period_ms) {}

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
  splash_.Play();
  runtime_.Render(display_);
  display_.Flush();
  for (;;) {
    midismith::os::Clock::delay_ms(tick_period_ms_);
    const std::int16_t rotation_detents = encoder_.ReadDeltaDetents();
    const auto button_event = button_.Poll();
    midismith::menu::InputEvent injected_event{};
    bool injected_event_received = injected_events_.Receive(injected_event, 0);
    const bool input_activity_detected =
        rotation_detents != 0 ||
        button_event != midismith::main_board::bsp::RotaryButton::Event::kNone ||
        injected_event_received;
    const bool wake_activity_detected = wake_activity_.ConsumeActivity();
    if (ProcessBacklightState(input_activity_detected || wake_activity_detected)) {
      continue;
    }
    DispatchRotation(rotation_detents);
    DispatchButton(button_event);
    while (injected_event_received) {
      runtime_.HandleInput(injected_event);
      injected_event_received = injected_events_.Receive(injected_event, 0);
    }
    if (runtime_.is_dirty()) {
      runtime_.Render(display_);
      display_.Flush();
    }
    if (midismith::main_board::app::config::kUiEncoderDebugOverlay) {
      RenderEncoderDebugOverlay();
    }
  }
}

void UiTask::RenderEncoderDebugOverlay() noexcept {
  constexpr std::uint8_t kCounterTextWidth = 5;
  std::array<char, 8> counter_text{};
  counter_text.fill(' ');
  static_cast<void>(std::to_chars(counter_text.data(), counter_text.data() + counter_text.size(),
                                  encoder_.raw_counter()));
  display_.DrawText(0, static_cast<std::uint8_t>(display_.columns() - kCounterTextWidth),
                    std::string_view(counter_text.data(), kCounterTextWidth),
                    midismith::text_display::CellAttribute::kWarning);
  display_.Flush();
}

bool UiTask::ProcessBacklightState(bool input_activity_detected) noexcept {
  if (input_activity_detected) {
    idle_tracker_.NoteActivity();
    if (backlight_off_) {
      backlight_off_ = false;
      display_power_.SetBacklight(true);
      return true;
    }
    return false;
  }
  if (idle_tracker_.Tick()) {
    backlight_off_ = true;
    display_power_.SetBacklight(false);
  }
  return backlight_off_;
}

bool UiTask::start() noexcept {
  return midismith::os::Task::create("UiTask", UiTask::entry, this,
                                     midismith::main_board::app::config::kUiTaskStackBytes,
                                     midismith::main_board::app::config::kUiTaskPriority);
}

void UiTask::DispatchRotation(std::int16_t detents) noexcept {
  if (detents != 0) {
    runtime_.HandleInput(midismith::menu::InputEvent::Rotate(detents));
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

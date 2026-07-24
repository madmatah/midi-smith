#pragma once

#include <cstdint>

#include "app/ui/activity_source_requirements.hpp"
#include "app/ui/display_power_requirements.hpp"
#include "app/ui/idle_tracker.hpp"
#include "app/ui/splash_requirements.hpp"
#include "bsp-types/input/button_source_requirements.hpp"
#include "bsp-types/input/rotation_source_requirements.hpp"
#include "menu/menu_runtime.hpp"
#include "os/queue_requirements.hpp"
#include "text-display/text_display_requirements.hpp"

namespace midismith::main_board::app::ui {

class UiTask {
 public:
  using InitializeCallback = void (*)(void* context) noexcept;

  UiTask(midismith::bsp::input::RotationSourceRequirements& encoder,
         midismith::bsp::input::ButtonSourceRequirements& button,
         midismith::menu::MenuRuntime& runtime,
         midismith::text_display::TextDisplayRequirements& display,
         DisplayPowerRequirements& display_power, SplashRequirements& splash,
         midismith::os::QueueRequirements<midismith::menu::InputEvent>& injected_events,
         ActivitySourceRequirements& wake_activity, std::uint32_t tick_period_ms,
         InitializeCallback initialize_callback, void* initialize_context) noexcept;

  static void entry(void* context) noexcept;
  void run() noexcept;
  bool start() noexcept;

 private:
  void RenderEncoderDebugOverlay() noexcept;
  bool ProcessBacklightState(bool input_activity_detected) noexcept;
  void DispatchRotation(std::int16_t detents) noexcept;
  void DispatchButton(midismith::bsp::input::ButtonEvent event) noexcept;

  midismith::bsp::input::RotationSourceRequirements& encoder_;
  midismith::bsp::input::ButtonSourceRequirements& button_;
  midismith::menu::MenuRuntime& runtime_;
  midismith::text_display::TextDisplayRequirements& display_;
  DisplayPowerRequirements& display_power_;
  SplashRequirements& splash_;
  midismith::os::QueueRequirements<midismith::menu::InputEvent>& injected_events_;
  ActivitySourceRequirements& wake_activity_;
  std::uint32_t tick_period_ms_;
  InitializeCallback initialize_callback_;
  void* initialize_context_;
  IdleTracker idle_tracker_;
  bool backlight_off_ = false;
};

}  // namespace midismith::main_board::app::ui

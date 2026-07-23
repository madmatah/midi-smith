#pragma once

#include <cstdint>

#include "bsp/rotary_button.hpp"
#include "bsp/rotary_encoder.hpp"
#include "menu/menu_runtime.hpp"
#include "text-display/text_display_requirements.hpp"

namespace midismith::main_board::app::ui {

class UiTask {
 public:
  using InitializeCallback = void (*)(void* context) noexcept;

  UiTask(midismith::main_board::bsp::RotaryEncoder& encoder,
         midismith::main_board::bsp::RotaryButton& button, midismith::menu::MenuRuntime& runtime,
         midismith::text_display::TextDisplayRequirements& display, std::uint32_t tick_period_ms,
         InitializeCallback initialize_callback, void* initialize_context) noexcept;

  static void entry(void* context) noexcept;
  void run() noexcept;
  bool start() noexcept;

 private:
  void DispatchRotation(std::int16_t detents) noexcept;
  void DispatchButton(midismith::main_board::bsp::RotaryButton::Event event) noexcept;

  midismith::main_board::bsp::RotaryEncoder& encoder_;
  midismith::main_board::bsp::RotaryButton& button_;
  midismith::menu::MenuRuntime& runtime_;
  midismith::text_display::TextDisplayRequirements& display_;
  std::uint32_t tick_period_ms_;
  InitializeCallback initialize_callback_;
  void* initialize_context_;
};

}  // namespace midismith::main_board::app::ui

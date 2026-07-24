#include "app/composition/ui_subsystem.hpp"

#include <array>

#include "app/config/ui.hpp"
#include "app/ui/menu_tree.hpp"
#include "app/ui/tft_text_display.hpp"
#include "app/ui/ui_task.hpp"
#include "bsp/board.hpp"
#include "bsp/rotary_button.hpp"
#include "bsp/rotary_encoder.hpp"
#include "bsp/tft_display.hpp"
#include "os/clock.hpp"

namespace midismith::main_board::app::composition {

namespace {

void DelayMs(void* context, std::uint32_t milliseconds) noexcept {
  static_cast<void>(context);
  midismith::os::Clock::delay_ms(milliseconds);
}

void InitializeTftDisplay(void* context) noexcept {
  static_cast<midismith::main_board::bsp::TftDisplay*>(context)->Init();
}

}  // namespace

void CreateUiSubsystem(ConfigContext& config, CalibrationContext& calibration,
                       ShellCommandsContext& commands) noexcept {
  static midismith::main_board::bsp::TftDisplay tft_display(
      midismith::main_board::bsp::Board::spi4_handle(),
      midismith::main_board::bsp::Board::lcd_chip_select(),
      midismith::main_board::bsp::Board::lcd_data_command(),
      midismith::main_board::bsp::Board::lcd_backlight(), DelayMs, nullptr);
  static midismith::main_board::app::ui::TftTextDisplay text_display(tft_display);
  static midismith::main_board::bsp::RotaryEncoder encoder(
      midismith::main_board::bsp::Board::tim2_handle());
  static midismith::main_board::bsp::RotaryButton button(
      midismith::main_board::bsp::Board::rotary_button_gpio(),
      midismith::main_board::app::config::kUiButtonDebounceReads,
      midismith::main_board::app::config::kUiButtonLongPressReads);
  static auto& root_screen =
      midismith::main_board::app::ui::BuildMenuTree(config, calibration, commands);
  static std::array<midismith::menu::MenuScreenRequirements*,
                    midismith::main_board::app::config::kMenuStackMaxDepth>
      menu_stack_storage{};
  static midismith::menu::MenuRuntime runtime(root_screen, menu_stack_storage.data(),
                                              menu_stack_storage.size());
  static midismith::main_board::app::ui::UiTask ui_task(
      encoder, button, runtime, text_display, text_display,
      midismith::main_board::app::config::kUiTickPeriodMs, InitializeTftDisplay, &tft_display);

  (void) ui_task.start();
}

}  // namespace midismith::main_board::app::composition

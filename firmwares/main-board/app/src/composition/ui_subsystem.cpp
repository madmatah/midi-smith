#include "app/composition/ui_subsystem.hpp"

#include <array>

#include "app/config/ui.hpp"
#include "app/config/ui_validation.hpp"
#include "app/shell/ui_command.hpp"
#include "app/tasks/shell_task.hpp"
#include "app/ui/menu_tree.hpp"
#include "app/ui/midi_activity_wake_source.hpp"
#include "app/ui/tft_splash_player.hpp"
#include "app/ui/tft_text_display.hpp"
#include "app/ui/ui_task.hpp"
#include "bsp/board.hpp"
#include "bsp/memory_sections.hpp"
#include "bsp/rotary_button.hpp"
#include "bsp/rotary_encoder.hpp"
#include "bsp/tft_display.hpp"
#include "os/clock.hpp"
#include "os/clock_delay.hpp"
#include "os/os_uptime_provider.hpp"
#include "os/queue.hpp"
#include "os/task.hpp"
#include "splash/animation.hpp"
#include "splash/pixel_canvas.hpp"

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
                       ShellCommandsContext& commands, MidiContext& midi) noexcept {
  static midismith::main_board::bsp::TftDisplay tft_display(
      midismith::main_board::bsp::Board::spi4_handle(),
      midismith::main_board::bsp::Board::lcd_chip_select(),
      midismith::main_board::bsp::Board::lcd_data_command(),
      midismith::main_board::bsp::Board::lcd_backlight(), DelayMs, nullptr);
  static std::array<std::uint16_t, midismith::main_board::app::ui::TftTextDisplay::kPixelCount>
      ui_framebuffer{};
  BSP_AXI_SRAM static std::array<std::uint16_t,
                                 midismith::main_board::app::ui::TftTextDisplay::kPixelCount>
      ui_transition_snapshot;
  static midismith::main_board::app::ui::TftTextDisplay text_display(
      tft_display, tft_display, ui_framebuffer.data(), ui_transition_snapshot.data());
  BSP_AXI_SRAM static std::array<std::uint8_t,
                                 midismith::splash::PixelCanvas::BandBufferBytes(
                                     midismith::splash::kDisplayWidth,
                                     midismith::main_board::app::config::kSplashBandRows)>
      splash_band_pixels;
  BSP_AXI_SRAM static std::array<std::uint8_t,
                                 static_cast<std::size_t>(midismith::splash::kDisplayWidth) *
                                     midismith::main_board::app::config::kSplashBandRows * 3>
      splash_band_row_pixels;
  BSP_AXI_SRAM static std::array<std::uint16_t,
                                 static_cast<std::size_t>(midismith::splash::kDisplayWidth) *
                                     midismith::main_board::app::config::kSplashBandRows>
      splash_band_row_colors;
  static midismith::os::ClockDelay ui_delay;
  static midismith::os::OsUptimeProvider ui_uptime;
  static midismith::main_board::app::ui::TftSplashPlayer splash_player(
      tft_display, midismith::main_board::app::config::kSplashBandRows, splash_band_pixels,
      splash_band_row_pixels, splash_band_row_colors, ui_delay, ui_uptime,
      midismith::main_board::app::config::kSplashFramePeriodMs,
      midismith::main_board::app::config::kSplashSaturationPercent);
  static midismith::main_board::bsp::RotaryEncoder encoder(
      midismith::main_board::bsp::Board::tim2_handle());
  static midismith::main_board::bsp::RotaryButton button(
      midismith::main_board::bsp::Board::rotary_button_gpio(),
      midismith::main_board::app::config::kUiButtonDebounceReads,
      midismith::main_board::app::config::kUiButtonLongPressReads);
  static auto menu_tree =
      midismith::main_board::app::ui::BuildMenuTree(config, calibration, commands, midi);
  static std::array<midismith::menu::MenuScreenRequirements*,
                    midismith::main_board::app::config::kMenuStackMaxDepth>
      menu_stack_storage{};
  static midismith::menu::MenuRuntime runtime(menu_tree.root, menu_stack_storage.data(),
                                              menu_stack_storage.size());
  runtime.set_navigation_observer(text_display);
  static midismith::os::Queue<midismith::menu::InputEvent,
                              midismith::main_board::app::config::kUiInjectedEventQueueDepth>
      injected_input_queue;
  static midismith::main_board::app::shell::UiCommand ui_command(injected_input_queue);
  (void) commands.task.RegisterCommand(ui_command);
  static midismith::main_board::app::ui::MidiActivityWakeSource midi_activity_wake_source(
      midi.activity, runtime, menu_tree.midi_monitor);
  static midismith::main_board::app::ui::UiTask ui_task(
      encoder, button, runtime, text_display, text_display, splash_player, injected_input_queue,
      midi_activity_wake_source, ui_delay, midismith::main_board::app::config::kUiTickPeriodMs,
      InitializeTftDisplay, &tft_display);

  (void) midismith::os::Task::create("UiTask", midismith::main_board::app::ui::UiTask::entry,
                                     &ui_task,
                                     midismith::main_board::app::config::kUiTaskStackBytes,
                                     midismith::main_board::app::config::kUiTaskPriority);
}

}  // namespace midismith::main_board::app::composition

#include "app/ui/screens/keymap_progress_screen.hpp"

#include <array>
#include <charconv>
#include <string_view>

#include "menu/menu_controller_requirements.hpp"
#include "menu/progress_bar.hpp"
#include "menu/text_layout.hpp"
#include "text-display/text_display_requirements.hpp"

namespace midismith::main_board::app::ui::screens {

namespace {

constexpr std::string_view kWidestCapturedCountText = "255/255";

using CapturedCountBuffer = std::array<char, kWidestCapturedCountText.size()>;

std::string_view FormatCapturedCount(std::uint8_t captured_count, std::uint8_t key_count,
                                     CapturedCountBuffer& buffer) noexcept {
  char* const buffer_end = buffer.data() + buffer.size();
  auto result = std::to_chars(buffer.data(), buffer_end, captured_count);
  *result.ptr = '/';
  result = std::to_chars(result.ptr + 1, buffer_end, key_count);
  return std::string_view(buffer.data(), static_cast<std::size_t>(result.ptr - buffer.data()));
}

}  // namespace

KeymapProgressScreen::KeymapProgressScreen(
    midismith::main_board::app::keymap::KeymapSetupCoordinator& coordinator) noexcept
    : coordinator_(coordinator) {}

void KeymapProgressScreen::OnEnter(
    midismith::menu::MenuControllerRequirements& controller) noexcept {
  static_cast<void>(controller);
}

bool KeymapProgressScreen::HandleInput(
    midismith::menu::InputEvent event,
    midismith::menu::MenuControllerRequirements& controller) noexcept {
  if (event.kind == midismith::menu::InputEvent::Kind::kButtonPress ||
      event.kind == midismith::menu::InputEvent::Kind::kButtonLongPress) {
    coordinator_.CancelSetup();
    controller.Pop();
    return true;
  }
  return false;
}

void KeymapProgressScreen::Render(
    midismith::text_display::TextDisplayRequirements& display) noexcept {
  const auto& session = coordinator_.session();
  CapturedCountBuffer captured_text{};
  const std::string_view rendered_captured =
      FormatCapturedCount(session.captured_count(), session.key_count(), captured_text);

  constexpr std::string_view kTitle = "Keymap";
  constexpr std::string_view kPrompt = "Press each key";
  const std::uint8_t footer_row = static_cast<std::uint8_t>(display.rows() - 1);

  display.Clear();
  display.FillRow(0, midismith::text_display::CellAttribute::kTitle);
  display.DrawText(0, midismith::menu::CenteredColumn(display.columns(), kTitle.size()), kTitle,
                   midismith::text_display::CellAttribute::kTitle);

  if (session.key_count() > 0 && session.captured_count() >= session.key_count()) {
    constexpr std::string_view kDoneLabel = "Done";
    const std::uint8_t done_row = static_cast<std::uint8_t>((display.rows() - 2) / 2);
    display.DrawTextDoubleSize(
        done_row, midismith::menu::CenteredColumn(display.columns(), kDoneLabel.size() * 2),
        kDoneLabel, midismith::text_display::CellAttribute::kSuccess);
    display.FillRow(footer_row, midismith::text_display::CellAttribute::kFooter);
    display.DrawText(footer_row, 1, "Btn exit", midismith::text_display::CellAttribute::kFooter);
    return;
  }

  const std::uint8_t prompt_row = static_cast<std::uint8_t>(display.rows() / 4);
  const std::uint8_t bar_row = static_cast<std::uint8_t>(display.rows() / 2);
  display.DrawText(prompt_row, midismith::menu::CenteredColumn(display.columns(), kPrompt.size()),
                   kPrompt, midismith::text_display::CellAttribute::kAccent);
  midismith::menu::RenderProgressBar(display, bar_row, 1,
                                     static_cast<std::uint8_t>(display.columns() - 2),
                                     session.captured_count(), session.key_count());
  display.DrawText(static_cast<std::uint8_t>(bar_row + 1),
                   midismith::menu::CenteredColumn(display.columns(), rendered_captured.size()),
                   rendered_captured, midismith::text_display::CellAttribute::kDim);
  display.FillRow(footer_row, midismith::text_display::CellAttribute::kFooter);
  display.DrawText(footer_row, 1, "Btn cancel", midismith::text_display::CellAttribute::kFooter);
}

bool KeymapProgressScreen::is_dirty() const noexcept {
  return true;
}

}  // namespace midismith::main_board::app::ui::screens

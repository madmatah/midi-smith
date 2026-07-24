#include "app/ui/screens/keymap_progress_screen.hpp"

#include <array>
#include <charconv>
#include <string_view>

#include "menu/menu_controller_requirements.hpp"
#include "menu/progress_bar.hpp"
#include "menu/text_layout.hpp"
#include "menu/title_bar.hpp"
#include "text-display/text_display_requirements.hpp"

namespace midismith::main_board::app::ui::screens {

KeymapProgressScreen::KeymapProgressScreen(
    midismith::main_board::app::keymap::KeymapSetupCoordinator& coordinator) noexcept
    : coordinator_(coordinator) {}

std::string_view KeymapProgressScreen::title() const noexcept {
  return "Keymap";
}

void KeymapProgressScreen::OnEnter(
    midismith::menu::MenuControllerRequirements& controller) noexcept {
  parent_title_ = controller.parent_title();
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
  std::array<char, 16> captured_text{};
  auto result = std::to_chars(captured_text.data(), captured_text.data() + captured_text.size(),
                              session.captured_count());
  *result.ptr = '/';
  result = std::to_chars(result.ptr + 1, captured_text.data() + captured_text.size(),
                         session.key_count());
  const std::string_view rendered_captured(
      captured_text.data(), static_cast<std::size_t>(result.ptr - captured_text.data()));

  constexpr std::string_view kPrompt = "Press each key";
  const std::uint8_t footer_row = static_cast<std::uint8_t>(display.rows() - 1);

  display.Clear();
  midismith::menu::RenderTitleBar(display, parent_title_, title());

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

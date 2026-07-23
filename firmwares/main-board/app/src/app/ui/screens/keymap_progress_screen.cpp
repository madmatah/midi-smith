#include "app/ui/screens/keymap_progress_screen.hpp"

#include <array>
#include <charconv>

#include "io/stream_format.hpp"
#include "menu/menu_controller_requirements.hpp"
#include "text-display/text_display_requirements.hpp"

namespace midismith::main_board::app::ui::screens {

KeymapProgressScreen::KeymapProgressScreen(
    midismith::main_board::app::keymap::KeymapSetupCoordinator& coordinator) noexcept
    : coordinator_(coordinator) {}

void KeymapProgressScreen::OnEnter(
    midismith::menu::MenuControllerRequirements& controller) noexcept {
  static_cast<void>(controller);
}

void KeymapProgressScreen::HandleInput(
    midismith::menu::InputEvent event,
    midismith::menu::MenuControllerRequirements& controller) noexcept {
  if (event.kind == midismith::menu::InputEvent::Kind::kButtonPress ||
      event.kind == midismith::menu::InputEvent::Kind::kButtonLongPress) {
    coordinator_.CancelSetup();
    controller.Pop();
  }
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

  display.Clear();
  display.DrawText(0, 0, "Keymap", midismith::text_display::CellAttribute::kDim);
  display.DrawText(2, 0, "Captured");
  display.DrawText(3, 0, std::string_view(captured_text.data()));
  display.DrawText(5, 0, "Press btn cancel");
}

bool KeymapProgressScreen::is_dirty() const noexcept {
  return true;
}

}  // namespace midismith::main_board::app::ui::screens

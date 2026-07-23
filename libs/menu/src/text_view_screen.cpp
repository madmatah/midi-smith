#include "menu/text_view_screen.hpp"

#include "menu/menu_controller_requirements.hpp"
#include "text-display/text_display_requirements.hpp"

namespace midismith::menu {

TextViewScreen::TextViewScreen(std::string_view title, LineBuffer& buffer) noexcept
    : title_(title), buffer_(buffer) {}

void TextViewScreen::OnEnter(MenuControllerRequirements& controller) noexcept {
  static_cast<void>(controller);
  first_visible_line_ = 0;
  dirty_ = true;
}

void TextViewScreen::HandleInput(InputEvent event,
                                 MenuControllerRequirements& controller) noexcept {
  if (event.kind == InputEvent::Kind::kRotate) {
    AdjustScroll(event.detents, 0);
    return;
  }
  if (event.kind == InputEvent::Kind::kButtonPress) {
    controller.Pop();
  }
}

void TextViewScreen::Render(midismith::text_display::TextDisplayRequirements& display) noexcept {
  display.Clear();
  display.DrawText(0, 0, title_, midismith::text_display::CellAttribute::kDim);
  const std::uint8_t visible_line_count = display.rows() > 1 ? display.rows() - 1 : 0;
  AdjustScroll(0, visible_line_count);
  for (std::uint8_t row_offset = 0; row_offset < visible_line_count; row_offset++) {
    const std::size_t line_index = first_visible_line_ + row_offset;
    if (line_index >= buffer_.line_count()) {
      break;
    }
    display.DrawText(static_cast<std::uint8_t>(row_offset + 1), 0, buffer_.line(line_index));
  }
  dirty_ = false;
}

bool TextViewScreen::is_dirty() const noexcept {
  return dirty_;
}

std::size_t TextViewScreen::first_visible_line() const noexcept {
  return first_visible_line_;
}

void TextViewScreen::AdjustScroll(std::int16_t detents, std::uint8_t visible_line_count) noexcept {
  const std::size_t previous_first_visible_line = first_visible_line_;
  if (detents > 0) {
    first_visible_line_ += static_cast<std::size_t>(detents);
  } else if (detents < 0) {
    const std::size_t absolute_detents = static_cast<std::size_t>(-detents);
    first_visible_line_ =
        absolute_detents > first_visible_line_ ? 0 : first_visible_line_ - absolute_detents;
  }
  if (visible_line_count > 0 && buffer_.line_count() > visible_line_count) {
    const std::size_t max_first_line = buffer_.line_count() - visible_line_count;
    if (first_visible_line_ > max_first_line) {
      first_visible_line_ = max_first_line;
    }
  } else if (visible_line_count > 0) {
    first_visible_line_ = 0;
  }
  dirty_ = dirty_ || previous_first_visible_line != first_visible_line_;
}

}  // namespace midismith::menu

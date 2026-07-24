#include "menu/text_view_screen.hpp"

#include <string_view>

#include "menu/menu_controller_requirements.hpp"
#include "menu/title_bar.hpp"
#include "text-display/glyphs.hpp"
#include "text-display/text_display_requirements.hpp"

namespace midismith::menu {

TextViewScreen::TextViewScreen(std::string_view title, LineBuffer& buffer) noexcept
    : title_(title), buffer_(buffer) {}

std::string_view TextViewScreen::title() const noexcept {
  return title_;
}

void TextViewScreen::OnEnter(MenuControllerRequirements& controller) noexcept {
  parent_title_ = controller.parent_title();
  first_visible_line_ = 0;
  dirty_ = true;
}

bool TextViewScreen::HandleInput(InputEvent event,
                                 MenuControllerRequirements& controller) noexcept {
  if (event.kind == InputEvent::Kind::kRotate) {
    AdjustScroll(event.detents, 0);
    return true;
  }
  if (event.kind == InputEvent::Kind::kButtonPress) {
    controller.Pop();
    return true;
  }
  return false;
}

void TextViewScreen::Render(midismith::text_display::TextDisplayRequirements& display) noexcept {
  namespace glyphs = midismith::text_display::glyphs;
  display.Clear();
  RenderTitleBar(display, parent_title_, title_);
  const std::uint8_t visible_line_count = display.rows() > 1 ? display.rows() - 1 : 0;
  AdjustScroll(0, visible_line_count);
  for (std::uint8_t row_offset = 0; row_offset < visible_line_count; row_offset++) {
    const std::size_t line_index = first_visible_line_ + row_offset;
    if (line_index >= buffer_.line_count()) {
      break;
    }
    display.DrawText(static_cast<std::uint8_t>(row_offset + 1), 0, buffer_.line(line_index));
  }
  const std::size_t total_line_count = buffer_.line_count();
  if (visible_line_count > 0 && total_line_count > visible_line_count) {
    const std::uint8_t scrollbar_column = static_cast<std::uint8_t>(display.columns() - 1);
    const std::uint8_t track_cells = visible_line_count;
    std::uint8_t thumb_cells =
        static_cast<std::uint8_t>(track_cells * visible_line_count / total_line_count);
    if (thumb_cells == 0) {
      thumb_cells = 1;
    }
    const std::uint8_t maximum_thumb_offset = static_cast<std::uint8_t>(track_cells - thumb_cells);
    const std::size_t maximum_first_line = total_line_count - visible_line_count;
    const std::uint8_t thumb_offset =
        static_cast<std::uint8_t>(maximum_thumb_offset * first_visible_line_ / maximum_first_line);
    for (std::uint8_t track_cell = 0; track_cell < track_cells; track_cell++) {
      const bool cell_in_thumb =
          track_cell >= thumb_offset && track_cell < thumb_offset + thumb_cells;
      const char scrollbar_glyph = cell_in_thumb ? glyphs::kScrollThumb : glyphs::kScrollTrack;
      const auto scrollbar_attribute = cell_in_thumb
                                           ? midismith::text_display::CellAttribute::kAccent
                                           : midismith::text_display::CellAttribute::kDim;
      display.DrawText(static_cast<std::uint8_t>(track_cell + 1), scrollbar_column,
                       std::string_view(&scrollbar_glyph, 1), scrollbar_attribute);
    }
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

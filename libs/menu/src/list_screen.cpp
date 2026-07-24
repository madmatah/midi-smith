#include "menu/list_screen.hpp"

#include <string_view>

#include "menu/menu_controller_requirements.hpp"
#include "menu/text_layout.hpp"
#include "text-display/glyphs.hpp"
#include "text-display/text_display_requirements.hpp"

namespace midismith::menu {

ListScreen::ListScreen(std::string_view title, MenuItemRequirements* const* items,
                       std::size_t item_count, bool wrap_navigation) noexcept
    : title_(title), items_(items), item_count_(item_count), wrap_navigation_(wrap_navigation) {}

void ListScreen::OnEnter(MenuControllerRequirements& controller) noexcept {
  static_cast<void>(controller);
  marquee_render_count_ = 0;
  dirty_ = true;
}

bool ListScreen::HandleInput(InputEvent event, MenuControllerRequirements& controller) noexcept {
  if (event.kind == InputEvent::Kind::kRotate) {
    MoveSelection(event.detents, 0);
    return true;
  }
  if (event.kind == InputEvent::Kind::kButtonPress && item_count_ > 0) {
    items_[selected_index_]->Activate(controller);
    return true;
  }
  return false;
}

void ListScreen::Render(midismith::text_display::TextDisplayRequirements& display) noexcept {
  namespace glyphs = midismith::text_display::glyphs;
  display.Clear();
  display.FillRow(0, midismith::text_display::CellAttribute::kTitle);
  display.DrawText(0, CenteredColumn(display.columns(), title_.size()), title_,
                   midismith::text_display::CellAttribute::kTitle);
  const std::uint8_t visible_item_count = display.rows() > 1 ? display.rows() - 1 : 0;
  AdjustVisibleWindow(visible_item_count);
  std::size_t last_visible_index = first_visible_index_;
  bool marquee_visible = false;
  for (std::uint8_t row_offset = 0; row_offset < visible_item_count; row_offset++) {
    const std::size_t item_index = first_visible_index_ + row_offset;
    if (item_index >= item_count_) {
      break;
    }
    last_visible_index = item_index;
    const bool item_selected = item_index == selected_index_;
    const auto attribute = item_selected ? midismith::text_display::CellAttribute::kHighlight
                                         : midismith::text_display::CellAttribute::kNormal;
    const std::uint8_t display_row = static_cast<std::uint8_t>(row_offset + 1);
    const char trailing_glyph = items_[item_index]->trailing_glyph();
    std::string_view label = items_[item_index]->label();
    const std::size_t label_width = static_cast<std::size_t>(
        display.columns() - 2 - (trailing_glyph != kNoTrailingGlyph ? 1 : 0));
    display.FillRow(display_row, attribute);
    if (item_selected && label.size() > label_width) {
      marquee_visible = true;
      const std::size_t overflow_pixels =
          (label.size() - label_width) * midismith::text_display::kGlyphWidthPixels;
      display.DrawTextScrolled(display_row, 1, static_cast<std::uint8_t>(label_width), label,
                               attribute,
                               static_cast<std::uint16_t>(MarqueeOffset(overflow_pixels)));
    } else {
      display.DrawText(display_row, 1, label.substr(0, label_width), attribute);
    }
    if (trailing_glyph != kNoTrailingGlyph && display.columns() >= 2) {
      display.DrawText(display_row, static_cast<std::uint8_t>(display.columns() - 2),
                       std::string_view(&trailing_glyph, 1), attribute);
    }
  }
  const std::uint8_t indicator_column = static_cast<std::uint8_t>(display.columns() - 1);
  if (first_visible_index_ > 0) {
    const auto attribute = first_visible_index_ == selected_index_
                               ? midismith::text_display::CellAttribute::kHighlight
                               : midismith::text_display::CellAttribute::kDim;
    display.DrawText(1, indicator_column, std::string_view(&glyphs::kArrowUp, 1), attribute);
  }
  if (first_visible_index_ + visible_item_count < item_count_) {
    const auto attribute = last_visible_index == selected_index_
                               ? midismith::text_display::CellAttribute::kHighlight
                               : midismith::text_display::CellAttribute::kDim;
    display.DrawText(static_cast<std::uint8_t>(display.rows() - 1), indicator_column,
                     std::string_view(&glyphs::kArrowDown, 1), attribute);
  }
  marquee_active_ = marquee_visible;
  if (marquee_active_) {
    marquee_render_count_++;
  }
  dirty_ = false;
}

std::size_t ListScreen::MarqueeOffset(std::size_t overflow_pixels) const noexcept {
  const std::uint32_t travel_renders =
      static_cast<std::uint32_t>((overflow_pixels + kMarqueeStepPixels - 1) / kMarqueeStepPixels);
  const std::uint32_t cycle_renders =
      static_cast<std::uint32_t>(2 * kMarqueePauseRenders) + travel_renders;
  const std::uint32_t cycle_position = marquee_render_count_ % cycle_renders;
  if (cycle_position < kMarqueePauseRenders) {
    return 0;
  }
  const std::size_t travelled_pixels =
      static_cast<std::size_t>(cycle_position - kMarqueePauseRenders) * kMarqueeStepPixels;
  return travelled_pixels < overflow_pixels ? travelled_pixels : overflow_pixels;
}

bool ListScreen::is_dirty() const noexcept {
  return dirty_ || marquee_active_;
}

std::size_t ListScreen::selected_index() const noexcept {
  return selected_index_;
}

std::size_t ListScreen::first_visible_index() const noexcept {
  return first_visible_index_;
}

void ListScreen::MoveSelection(std::int16_t detents, std::uint8_t visible_item_count) noexcept {
  static_cast<void>(visible_item_count);
  if (item_count_ == 0 || detents == 0) {
    return;
  }
  const std::size_t previous_index = selected_index_;
  if (detents > 0) {
    for (std::int16_t step = 0; step < detents; step++) {
      if (selected_index_ + 1 < item_count_) {
        selected_index_++;
      } else if (wrap_navigation_) {
        selected_index_ = 0;
      }
    }
  } else {
    for (std::int16_t step = 0; step > detents; step--) {
      if (selected_index_ > 0) {
        selected_index_--;
      } else if (wrap_navigation_) {
        selected_index_ = item_count_ - 1;
      }
    }
  }
  if (previous_index != selected_index_) {
    marquee_render_count_ = 0;
    dirty_ = true;
  }
}

void ListScreen::AdjustVisibleWindow(std::uint8_t visible_item_count) noexcept {
  if (visible_item_count == 0 || item_count_ == 0) {
    first_visible_index_ = 0;
    return;
  }
  if (selected_index_ < first_visible_index_) {
    first_visible_index_ = selected_index_;
  }
  const std::size_t last_visible_index = first_visible_index_ + visible_item_count - 1;
  if (selected_index_ > last_visible_index) {
    first_visible_index_ = selected_index_ - visible_item_count + 1;
  }
}

}  // namespace midismith::menu

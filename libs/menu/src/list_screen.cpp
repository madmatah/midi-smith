#include "menu/list_screen.hpp"

#include "menu/menu_controller_requirements.hpp"
#include "text-display/text_display_requirements.hpp"

namespace midismith::menu {

ListScreen::ListScreen(std::string_view title, MenuItemRequirements* const* items,
                       std::size_t item_count, bool wrap_navigation) noexcept
    : title_(title), items_(items), item_count_(item_count), wrap_navigation_(wrap_navigation) {}

void ListScreen::OnEnter(MenuControllerRequirements& controller) noexcept {
  static_cast<void>(controller);
  dirty_ = true;
}

void ListScreen::HandleInput(InputEvent event, MenuControllerRequirements& controller) noexcept {
  if (event.kind == InputEvent::Kind::kRotate) {
    MoveSelection(event.detents, 0);
    return;
  }
  if (event.kind == InputEvent::Kind::kButtonPress && item_count_ > 0) {
    items_[selected_index_]->Activate(controller);
  }
}

void ListScreen::Render(midismith::text_display::TextDisplayRequirements& display) noexcept {
  display.Clear();
  display.DrawText(0, 0, title_, midismith::text_display::CellAttribute::kDim);
  const std::uint8_t visible_item_count = display.rows() > 1 ? display.rows() - 1 : 0;
  AdjustVisibleWindow(visible_item_count);
  for (std::uint8_t row_offset = 0; row_offset < visible_item_count; row_offset++) {
    const std::size_t item_index = first_visible_index_ + row_offset;
    if (item_index >= item_count_) {
      break;
    }
    const auto attribute = item_index == selected_index_
                               ? midismith::text_display::CellAttribute::kHighlight
                               : midismith::text_display::CellAttribute::kNormal;
    display.FillRow(static_cast<std::uint8_t>(row_offset + 1), attribute);
    display.DrawText(static_cast<std::uint8_t>(row_offset + 1), 0, items_[item_index]->label(),
                     attribute);
  }
  dirty_ = false;
}

bool ListScreen::is_dirty() const noexcept {
  return dirty_;
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
  dirty_ = dirty_ || previous_index != selected_index_;
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

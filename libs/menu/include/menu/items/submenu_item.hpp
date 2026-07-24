#pragma once

#include "menu/menu_controller_requirements.hpp"
#include "menu/menu_item_requirements.hpp"
#include "menu/menu_screen_requirements.hpp"
#include "text-display/glyphs.hpp"

namespace midismith::menu::items {

class SubmenuItem final : public midismith::menu::MenuItemRequirements {
 public:
  SubmenuItem(std::string_view label, midismith::menu::MenuScreenRequirements& screen) noexcept
      : label_(label), screen_(screen) {}

  std::string_view label() const noexcept override {
    return label_;
  }

  char trailing_glyph() const noexcept override {
    return midismith::text_display::glyphs::kChevronRight;
  }

  void Activate(midismith::menu::MenuControllerRequirements& controller) noexcept override {
    controller.Push(screen_);
  }

 private:
  std::string_view label_;
  midismith::menu::MenuScreenRequirements& screen_;
};

}  // namespace midismith::menu::items

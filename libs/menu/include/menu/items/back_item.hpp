#pragma once

#include "menu/menu_controller_requirements.hpp"
#include "menu/menu_item_requirements.hpp"
#include "text-display/glyphs.hpp"

namespace midismith::menu::items {

inline constexpr char kBackLabelText[] = {
    midismith::text_display::glyphs::kArrowLeft, ' ', 'B', 'a', 'c', 'k', '\0'};

class BackItem final : public midismith::menu::MenuItemRequirements {
 public:
  explicit BackItem(std::string_view label = kBackLabelText) noexcept : label_(label) {}

  std::string_view label() const noexcept override {
    return label_;
  }

  void Activate(midismith::menu::MenuControllerRequirements& controller) noexcept override {
    controller.Pop();
  }

 private:
  std::string_view label_;
};

}  // namespace midismith::menu::items

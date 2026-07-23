#pragma once

#include "menu/menu_controller_requirements.hpp"
#include "menu/menu_item_requirements.hpp"

namespace midismith::menu::items {

class BackItem final : public midismith::menu::MenuItemRequirements {
 public:
  explicit BackItem(std::string_view label = "<- Back") noexcept : label_(label) {}

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

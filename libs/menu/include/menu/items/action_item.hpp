#pragma once

#include "menu/menu_item_requirements.hpp"

namespace midismith::menu::items {

class ActionItem final : public midismith::menu::MenuItemRequirements {
 public:
  using Action = void (*)(void* context) noexcept;

  ActionItem(std::string_view label, Action action, void* action_context) noexcept
      : label_(label), action_(action), action_context_(action_context) {}

  std::string_view label() const noexcept override {
    return label_;
  }

  void Activate(midismith::menu::MenuControllerRequirements& controller) noexcept override {
    static_cast<void>(controller);
    if (action_ != nullptr) {
      action_(action_context_);
    }
  }

 private:
  std::string_view label_;
  Action action_;
  void* action_context_;
};

}  // namespace midismith::menu::items

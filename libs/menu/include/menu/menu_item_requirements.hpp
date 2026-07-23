#pragma once

#include <string_view>

namespace midismith::menu {

class MenuControllerRequirements;

class MenuItemRequirements {
 public:
  virtual ~MenuItemRequirements() = default;

  virtual std::string_view label() const noexcept = 0;
  virtual void Activate(MenuControllerRequirements& controller) noexcept = 0;
};

}  // namespace midismith::menu

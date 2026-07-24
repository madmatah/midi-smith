#pragma once

#include <string_view>

namespace midismith::menu {

class MenuScreenRequirements;

class MenuControllerRequirements {
 public:
  virtual ~MenuControllerRequirements() = default;

  virtual bool Push(MenuScreenRequirements& screen) noexcept = 0;
  virtual bool Pop() noexcept = 0;
  virtual std::string_view parent_title() const noexcept {
    return {};
  }
};

}  // namespace midismith::menu

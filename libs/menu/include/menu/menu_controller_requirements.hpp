#pragma once

namespace midismith::menu {

class MenuScreenRequirements;

class MenuControllerRequirements {
 public:
  virtual ~MenuControllerRequirements() = default;

  virtual bool Push(MenuScreenRequirements& screen) noexcept = 0;
  virtual bool Pop() noexcept = 0;
};

}  // namespace midismith::menu

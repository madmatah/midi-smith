#pragma once

namespace midismith::menu {

class MenuNavigationObserverRequirements {
 public:
  virtual ~MenuNavigationObserverRequirements() = default;

  virtual void OnScreenPushed() noexcept = 0;
  virtual void OnScreenPopped() noexcept = 0;
};

}  // namespace midismith::menu

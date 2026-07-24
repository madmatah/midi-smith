#pragma once

#include "menu/input_event.hpp"

namespace midismith::text_display {
class TextDisplayRequirements;
}

namespace midismith::menu {

class MenuControllerRequirements;

class MenuScreenRequirements {
 public:
  virtual ~MenuScreenRequirements() = default;

  virtual void OnEnter(MenuControllerRequirements& controller) noexcept = 0;
  virtual bool HandleInput(InputEvent event, MenuControllerRequirements& controller) noexcept = 0;
  virtual void Render(midismith::text_display::TextDisplayRequirements& display) noexcept = 0;
  virtual bool is_dirty() const noexcept = 0;
};

}  // namespace midismith::menu

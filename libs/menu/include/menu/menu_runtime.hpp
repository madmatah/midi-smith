#pragma once

#include "menu/input_event.hpp"
#include "menu/menu_controller_requirements.hpp"
#include "menu/menu_stack.hpp"

namespace midismith::text_display {
class TextDisplayRequirements;
}

namespace midismith::menu {

class MenuNavigationObserverRequirements;

class MenuRuntime final : public MenuControllerRequirements {
 public:
  MenuRuntime(MenuScreenRequirements& root_screen, MenuScreenRequirements** stack_storage,
              std::size_t stack_capacity) noexcept;

  void HandleInput(InputEvent event) noexcept;
  void Render(midismith::text_display::TextDisplayRequirements& display) noexcept;
  bool is_dirty() const noexcept;
  const MenuScreenRequirements* current_screen() const noexcept;
  void set_navigation_observer(MenuNavigationObserverRequirements& observer) noexcept;

  bool Push(MenuScreenRequirements& screen) noexcept override;
  bool Pop() noexcept override;

 private:
  MenuStack stack_;
  MenuNavigationObserverRequirements* navigation_observer_ = nullptr;
  bool dirty_ = true;
};

}  // namespace midismith::menu

#pragma once

#include "menu/input_event.hpp"
#include "menu/menu_controller_requirements.hpp"
#include "menu/menu_stack.hpp"

namespace midismith::text_display {
class TextDisplayRequirements;
}

namespace midismith::menu {

class MenuRuntime final : public MenuControllerRequirements {
 public:
  MenuRuntime(MenuScreenRequirements& root_screen, MenuScreenRequirements** stack_storage,
              std::size_t stack_capacity) noexcept;

  void HandleInput(InputEvent event) noexcept;
  void Render(midismith::text_display::TextDisplayRequirements& display) noexcept;
  bool is_dirty() const noexcept;

  bool Push(MenuScreenRequirements& screen) noexcept override;
  bool Pop() noexcept override;
  std::string_view parent_title() const noexcept override;

 private:
  MenuStack stack_;
  bool dirty_ = true;
};

}  // namespace midismith::menu

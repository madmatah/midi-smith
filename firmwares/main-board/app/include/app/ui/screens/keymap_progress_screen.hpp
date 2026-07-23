#pragma once

#include "app/keymap/keymap_setup_coordinator.hpp"
#include "menu/menu_screen_requirements.hpp"

namespace midismith::main_board::app::ui::screens {

class KeymapProgressScreen final : public midismith::menu::MenuScreenRequirements {
 public:
  explicit KeymapProgressScreen(
      midismith::main_board::app::keymap::KeymapSetupCoordinator& coordinator) noexcept;

  void OnEnter(midismith::menu::MenuControllerRequirements& controller) noexcept override;
  void HandleInput(midismith::menu::InputEvent event,
                   midismith::menu::MenuControllerRequirements& controller) noexcept override;
  void Render(midismith::text_display::TextDisplayRequirements& display) noexcept override;
  bool is_dirty() const noexcept override;

 private:
  midismith::main_board::app::keymap::KeymapSetupCoordinator& coordinator_;
};

}  // namespace midismith::main_board::app::ui::screens

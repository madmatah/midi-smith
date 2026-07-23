#pragma once

#include "app/shell/calibration_coordinator_requirements.hpp"
#include "menu/menu_screen_requirements.hpp"

namespace midismith::main_board::app::ui::screens {

class CalibrationProgressScreen final : public midismith::menu::MenuScreenRequirements {
 public:
  explicit CalibrationProgressScreen(
      midismith::main_board::app::shell::CalibrationCoordinatorRequirements& coordinator) noexcept;

  void OnEnter(midismith::menu::MenuControllerRequirements& controller) noexcept override;
  void HandleInput(midismith::menu::InputEvent event,
                   midismith::menu::MenuControllerRequirements& controller) noexcept override;
  void Render(midismith::text_display::TextDisplayRequirements& display) noexcept override;
  bool is_dirty() const noexcept override;

 private:
  midismith::main_board::app::shell::CalibrationCoordinatorRequirements& coordinator_;
  bool started_ = false;
};

}  // namespace midismith::main_board::app::ui::screens

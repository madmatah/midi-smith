#pragma once

#include <cstdint>

#include "app/shell/calibration_coordinator_requirements.hpp"
#include "menu/menu_screen_requirements.hpp"

namespace midismith::main_board::app::ui::screens {

class CalibrationProgressScreen final : public midismith::menu::MenuScreenRequirements {
 public:
  explicit CalibrationProgressScreen(
      midismith::main_board::app::shell::CalibrationCoordinatorRequirements& coordinator) noexcept;

  std::string_view title() const noexcept override;
  void OnEnter(midismith::menu::MenuControllerRequirements& controller) noexcept override;
  bool HandleInput(midismith::menu::InputEvent event,
                   midismith::menu::MenuControllerRequirements& controller) noexcept override;
  void Render(midismith::text_display::TextDisplayRequirements& display) noexcept override;
  bool is_dirty() const noexcept override;

 private:
  midismith::main_board::app::shell::CalibrationCoordinatorRequirements& coordinator_;
  std::string_view parent_title_{};
  bool started_ = false;
  std::uint16_t spinner_render_count_ = 0;
};

}  // namespace midismith::main_board::app::ui::screens

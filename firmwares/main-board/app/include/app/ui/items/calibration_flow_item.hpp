#pragma once

#include "menu/menu_item_requirements.hpp"
#include "menu/menu_screen_requirements.hpp"

namespace midismith::main_board::app::ui::items {

class CalibrationFlowItem final : public midismith::menu::MenuItemRequirements {
 public:
  explicit CalibrationFlowItem(midismith::menu::MenuScreenRequirements& progress_screen) noexcept;

  std::string_view label() const noexcept override;
  void Activate(midismith::menu::MenuControllerRequirements& controller) noexcept override;

 private:
  midismith::menu::MenuScreenRequirements& progress_screen_;
};

}  // namespace midismith::main_board::app::ui::items

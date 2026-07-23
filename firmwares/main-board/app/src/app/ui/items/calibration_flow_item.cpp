#include "app/ui/items/calibration_flow_item.hpp"

#include "menu/menu_controller_requirements.hpp"

namespace midismith::main_board::app::ui::items {

CalibrationFlowItem::CalibrationFlowItem(
    midismith::menu::MenuScreenRequirements& progress_screen) noexcept
    : progress_screen_(progress_screen) {}

std::string_view CalibrationFlowItem::label() const noexcept {
  return "Calibration";
}

void CalibrationFlowItem::Activate(
    midismith::menu::MenuControllerRequirements& controller) noexcept {
  controller.Push(progress_screen_);
}

}  // namespace midismith::main_board::app::ui::items

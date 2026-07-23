#pragma once

#include "app/composition/subsystems.hpp"
#include "menu/menu_screen_requirements.hpp"

namespace midismith::main_board::app::ui {

midismith::menu::MenuScreenRequirements& BuildMenuTree(
    midismith::main_board::app::composition::ConfigContext& config,
    midismith::main_board::app::composition::CalibrationContext& calibration,
    midismith::main_board::app::composition::ShellCommandsContext& commands) noexcept;

}

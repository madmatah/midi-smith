#pragma once

#include "app/composition/subsystems.hpp"
#include "menu/menu_screen_requirements.hpp"

namespace midismith::main_board::app::ui {

struct MenuTree {
  midismith::menu::MenuScreenRequirements& root;
  midismith::menu::MenuScreenRequirements& midi_monitor;
};

MenuTree BuildMenuTree(midismith::main_board::app::composition::ConfigContext& config,
                       midismith::main_board::app::composition::CalibrationContext& calibration,
                       midismith::main_board::app::composition::ShellCommandsContext& commands,
                       midismith::main_board::app::composition::MidiContext& midi) noexcept;

}  // namespace midismith::main_board::app::ui

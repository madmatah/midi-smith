#pragma once

#include "app/composition/subsystems.hpp"
#include "menu/menu_screen_requirements.hpp"

namespace midismith::main_board::app::composition {

struct MenuTree {
  midismith::menu::MenuScreenRequirements& root;
  midismith::menu::MenuScreenRequirements& midi_monitor;
};

MenuTree BuildMenuTree(ConfigContext& config, CalibrationContext& calibration,
                       ShellCommandsContext& commands, MidiContext& midi) noexcept;

}  // namespace midismith::main_board::app::composition

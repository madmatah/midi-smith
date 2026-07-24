#pragma once

#include "app/composition/subsystems.hpp"

namespace midismith::main_board::app::composition {

void CreateUiSubsystem(ConfigContext& config, CalibrationContext& calibration,
                       ShellCommandsContext& commands, MidiContext& midi) noexcept;

}

#pragma once

#include "io/stream_requirements.hpp"
#include "logging/logger_requirements.hpp"
#include "piano-controller/piano_requirements.hpp"

namespace midismith::main_board::app::composition {

struct ConsoleContext {
  midismith::io::StreamRequirements& stream;
};

struct MidiContext {
  midismith::piano_controller::PianoRequirements& piano;
};

MidiContext CreateMidiSubsystem(midismith::logging::LoggerRequirements& logger) noexcept;
void CreateLedSubsystem(MidiContext& midi) noexcept;
void CreateShellSubsystem(ConsoleContext& console) noexcept;

}  // namespace midismith::main_board::app::composition

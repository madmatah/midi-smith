#pragma once

#include <cstdint>

namespace midismith::main_board::app::midi {

/**
 * @brief Represents a standard 3-byte MIDI message.
 */
struct MidiCommand {
  uint8_t data[3];
  uint8_t length;
};

}  // namespace midismith::main_board::app::midi

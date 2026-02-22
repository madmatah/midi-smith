#pragma once
#include <cstdint>

namespace midismith::common::domain::music {

/**
 * @brief Unique identifier for a note in the chromatic scale.
 * Uses universal numbering (0-127) where 60 is middle C (C4).
 */
using NoteNumber = uint8_t;
inline constexpr NoteNumber kNoteC4 = 60;

/**
 * @brief Intensity of the action on a key (0-127).
 */
using Velocity = uint8_t;


}  // namespace midismith::common::domain::music

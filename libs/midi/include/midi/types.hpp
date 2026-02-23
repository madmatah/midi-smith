#pragma once

#include <cstdint>

namespace midismith::midi {

using NoteNumber = uint8_t;
inline constexpr NoteNumber kNoteC4 = 60;

using Velocity = uint8_t;

}  // namespace midismith::midi

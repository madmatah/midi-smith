#include "midi/note_name.hpp"

#include <array>
#include <cstdint>

namespace midismith::midi {

namespace {

constexpr std::array<std::string_view, 12> kPitchClassNames{"C",  "C#", "D",  "D#", "E",  "F",
                                                            "F#", "G",  "G#", "A",  "A#", "B"};
constexpr int kSemitonesPerOctave = 12;
constexpr int kLowestOctave = -1;
constexpr NoteNumber kDataByteMask = 0x7F;

}  // namespace

std::string_view FormatNoteName(NoteNumber note,
                                std::span<char, kNoteNameCapacity> buffer) noexcept {
  const int note_in_range = static_cast<int>(note & kDataByteMask);
  const std::string_view pitch_class_name =
      kPitchClassNames[static_cast<std::size_t>(note_in_range % kSemitonesPerOctave)];
  const int octave = note_in_range / kSemitonesPerOctave + kLowestOctave;

  std::size_t length = 0;
  for (const char character : pitch_class_name) {
    buffer[length] = character;
    length++;
  }
  if (octave < 0) {
    buffer[length] = '-';
    length++;
  }
  buffer[length] = static_cast<char>('0' + (octave < 0 ? -octave : octave));
  length++;

  return std::string_view(buffer.data(), length);
}

}  // namespace midismith::midi

#pragma once

#include <cstddef>
#include <span>
#include <string_view>

#include "midi/types.hpp"

namespace midismith::midi {

inline constexpr std::string_view kLongestNoteName = "C#-1";
inline constexpr std::size_t kNoteNameCapacity = kLongestNoteName.size();

std::string_view FormatNoteName(NoteNumber note,
                                std::span<char, kNoteNameCapacity> buffer) noexcept;

}  // namespace midismith::midi

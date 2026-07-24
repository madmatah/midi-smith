#pragma once

#include <cstdint>

namespace midismith::text_display::glyphs {

inline constexpr char kBarFillBase = '\x10';
inline constexpr char kBarFillFull = '\x18';
inline constexpr char kArrowUp = '\x19';
inline constexpr char kArrowDown = '\x1A';
inline constexpr char kChevronRight = '\x1B';
inline constexpr char kArrowLeft = '\x1C';
inline constexpr char kScrollTrack = '\x1D';
inline constexpr char kScrollThumb = '\x1E';

inline constexpr std::uint8_t kBarFillLevels = 8;

constexpr char BarFill(std::uint32_t eighths) noexcept {
  return static_cast<char>(kBarFillBase + (eighths > kBarFillLevels ? kBarFillLevels : eighths));
}

constexpr bool IsCustomGlyph(char character) noexcept {
  return character >= kBarFillBase && character <= kScrollThumb;
}

}  // namespace midismith::text_display::glyphs

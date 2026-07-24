#pragma once

#include <cstdint>
#include <span>

namespace midismith::main_board::app::ui {

inline constexpr std::uint8_t kGlyphSourceWidth = 8;
inline constexpr std::uint8_t kGlyphSourceHeight = 16;
inline constexpr std::uint8_t kLeftmostPixelMask = 0x80;

inline bool GlyphPixel(std::span<const std::uint8_t, kGlyphSourceHeight> glyph, int x,
                       int y) noexcept {
  if (x < 0 || x >= kGlyphSourceWidth || y < 0 || y >= kGlyphSourceHeight) {
    return false;
  }
  return (glyph[static_cast<std::size_t>(y)] & (kLeftmostPixelMask >> x)) != 0;
}

inline bool SampleScale2xGlyphPixel(std::span<const std::uint8_t, kGlyphSourceHeight> glyph,
                                    std::uint8_t target_x, std::uint8_t target_y) noexcept {
  const int source_x = target_x / 2;
  const int source_y = target_y / 2;
  const bool center = GlyphPixel(glyph, source_x, source_y);
  const bool up = GlyphPixel(glyph, source_x, source_y - 1);
  const bool left = GlyphPixel(glyph, source_x - 1, source_y);
  const bool right = GlyphPixel(glyph, source_x + 1, source_y);
  const bool down = GlyphPixel(glyph, source_x, source_y + 1);
  if (up == down || left == right) {
    return center;
  }
  const bool right_half = (target_x % 2) != 0;
  const bool bottom_half = (target_y % 2) != 0;

  if (!right_half && !bottom_half) {
    return left == up ? left : center;
  }
  if (right_half && !bottom_half) {
    return up == right ? right : center;
  }
  if (!right_half) {
    return left == down ? left : center;
  }
  return down == right ? right : center;
}

}  // namespace midismith::main_board::app::ui

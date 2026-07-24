#pragma once

#include <cstdint>

namespace midismith::splash {

struct Color {
  std::uint8_t red;
  std::uint8_t green;
  std::uint8_t blue;
};

inline constexpr Color kStringIvory{180, 173, 160};
inline constexpr Color kWarmIvory{244, 237, 222};
inline constexpr Color kFeltMidtone{202, 190, 169};
inline constexpr Color kFeltShadow{82, 73, 63};
inline constexpr Color kWood{105, 70, 43};
inline constexpr Color kWoodHighlight{165, 118, 73};
inline constexpr Color kUnderfeltInk{52, 56, 74};
inline constexpr Color kBrightIvory{255, 252, 244};

}  // namespace midismith::splash

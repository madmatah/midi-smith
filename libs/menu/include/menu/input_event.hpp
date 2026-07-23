#pragma once

#include <cstdint>

namespace midismith::menu {

struct InputEvent {
  enum class Kind : std::uint8_t {
    kRotate,
    kButtonPress,
    kButtonLongPress,
  };

  static constexpr InputEvent Rotate(std::int16_t detents) noexcept {
    return {.kind = Kind::kRotate, .detents = detents};
  }

  static constexpr InputEvent ButtonPress() noexcept {
    return {.kind = Kind::kButtonPress, .detents = 0};
  }

  static constexpr InputEvent ButtonLongPress() noexcept {
    return {.kind = Kind::kButtonLongPress, .detents = 0};
  }

  Kind kind;
  std::int16_t detents;
};

}  // namespace midismith::menu

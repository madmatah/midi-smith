#pragma once

#include <cstdint>

namespace midismith::bsp::input {

enum class ButtonEvent : std::uint8_t {
  kNone,
  kPressed,
  kReleased,
  kLongPressed,
};

class ButtonSourceRequirements {
 public:
  virtual ~ButtonSourceRequirements() = default;

  virtual ButtonEvent Poll() noexcept = 0;
};

}  // namespace midismith::bsp::input

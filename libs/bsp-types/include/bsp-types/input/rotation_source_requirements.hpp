#pragma once

#include <cstdint>

namespace midismith::bsp::input {

class RotationSourceRequirements {
 public:
  virtual ~RotationSourceRequirements() = default;

  virtual void Start() noexcept = 0;
  virtual std::int16_t ReadDeltaDetents() noexcept = 0;
  virtual std::uint16_t raw_counter() const noexcept = 0;
};

}  // namespace midismith::bsp::input

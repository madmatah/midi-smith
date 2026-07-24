#pragma once

namespace midismith::bsp::display {

class BacklightRequirements {
 public:
  virtual ~BacklightRequirements() = default;

  virtual void SetBacklight(bool enabled) noexcept = 0;
};

}  // namespace midismith::bsp::display

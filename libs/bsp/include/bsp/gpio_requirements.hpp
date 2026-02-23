#pragma once

namespace midismith::bsp {

class GpioRequirements {
 public:
  virtual ~GpioRequirements() = default;

  virtual void set() noexcept = 0;
  virtual void reset() noexcept = 0;
  virtual void toggle() noexcept = 0;
  virtual bool read() const noexcept = 0;
};

}  // namespace midismith::bsp

#pragma once

#include <cstdint>

#include "bsp/gpio_requirements.hpp"

namespace midismith::common::bsp {

class Gpio : public GpioRequirements {
 public:
  explicit Gpio(std::uintptr_t port, std::uint16_t pin) noexcept : port_(port), pin_(pin) {}

  void set() noexcept override;
  void reset() noexcept override;
  void toggle() noexcept override;
  bool read() const noexcept override;

 private:
  std::uintptr_t port_;
  std::uint16_t pin_;
};

}  // namespace midismith::common::bsp

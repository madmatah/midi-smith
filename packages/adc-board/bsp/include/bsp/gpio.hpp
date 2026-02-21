#pragma once

#include <cstdint>

#include "bsp/gpio_requirements.hpp"

namespace bsp {

class Gpio : public GpioRequirements {
 public:
  explicit Gpio(std::uintptr_t port, std::uint16_t pin) noexcept : _port(port), _pin(pin) {}

  void set() noexcept override;
  void reset() noexcept override;
  void toggle() noexcept override;
  bool read() const noexcept override;

 private:
  std::uintptr_t _port;
  std::uint16_t _pin;
};

}  // namespace bsp

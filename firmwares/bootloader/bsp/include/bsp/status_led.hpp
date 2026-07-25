#pragma once

#include <cstdint>

namespace midismith::bootloader::bsp {

class StatusLed {
 public:
  static void TurnOn() noexcept;
  static void TurnOff() noexcept;

  [[noreturn]] static void BlinkForever(std::uint32_t period_ms) noexcept;
};

}  // namespace midismith::bootloader::bsp

#pragma once

#include <cstdint>

#include "bsp/gpio_requirements.hpp"

namespace midismith::main_board::bsp {

class RotaryButton {
 public:
  enum class Event : std::uint8_t {
    kNone,
    kPressed,
    kReleased,
    kLongPressed,
  };

  RotaryButton(midismith::bsp::GpioRequirements& gpio, std::uint8_t debounce_reads,
               std::uint16_t long_press_reads) noexcept;

  Event Poll() noexcept;

 private:
  midismith::bsp::GpioRequirements& gpio_;
  std::uint8_t debounce_reads_;
  std::uint16_t long_press_reads_;
  bool stable_pressed_ = false;
  bool last_raw_pressed_ = false;
  bool long_press_reported_ = false;
  std::uint8_t stable_read_count_ = 0;
  std::uint16_t pressed_read_count_ = 0;
};

}  // namespace midismith::main_board::bsp

#include "bsp/rotary_button.hpp"

namespace midismith::main_board::bsp {

RotaryButton::RotaryButton(midismith::bsp::GpioRequirements& gpio, std::uint8_t debounce_reads,
                           std::uint16_t long_press_reads) noexcept
    : gpio_(gpio), debounce_reads_(debounce_reads), long_press_reads_(long_press_reads) {}

RotaryButton::Event RotaryButton::Poll() noexcept {
  const bool raw_pressed = !gpio_.read();
  if (raw_pressed == last_raw_pressed_) {
    if (stable_read_count_ < debounce_reads_) {
      stable_read_count_++;
    }
  } else {
    last_raw_pressed_ = raw_pressed;
    stable_read_count_ = 1;
  }
  if (stable_read_count_ < debounce_reads_ || raw_pressed == stable_pressed_) {
    if (stable_pressed_) {
      pressed_read_count_++;
      if (!long_press_reported_ && pressed_read_count_ >= long_press_reads_) {
        long_press_reported_ = true;
        return Event::kLongPressed;
      }
    }
    return Event::kNone;
  }
  stable_pressed_ = raw_pressed;
  if (stable_pressed_) {
    pressed_read_count_ = 0;
    long_press_reported_ = false;
    return Event::kPressed;
  }
  pressed_read_count_ = 0;
  const bool was_long_press_reported = long_press_reported_;
  long_press_reported_ = false;
  return was_long_press_reported ? Event::kNone : Event::kReleased;
}

}  // namespace midismith::main_board::bsp

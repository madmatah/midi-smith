#include "os/clock_delay.hpp"

#include "os/clock.hpp"

namespace midismith::os {

void ClockDelay::DelayMs(std::uint32_t milliseconds) noexcept {
  Clock::delay_ms(milliseconds);
}

}  // namespace midismith::os

#include "app/shell/os_clock_blocking_delay.hpp"

#include "os/clock.hpp"

namespace midismith::adc_board::app::shell {

void OsClockBlockingDelay::DelayMs(std::uint32_t delay_ms) noexcept {
  midismith::os::Clock::delay_ms(delay_ms);
}

}  // namespace midismith::adc_board::app::shell

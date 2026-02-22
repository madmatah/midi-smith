#include "os/clock.hpp"

#include "cmsis_os2.h"

namespace midismith::common::os {

void Clock::delay_ms(std::uint32_t ms) noexcept {
  osDelay(ms);
}

}  // namespace midismith::common::os

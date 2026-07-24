#include "os/clock.hpp"

#include "cmsis_os2.h"

namespace midismith::os {

void Clock::delay_ms(std::uint32_t ms) noexcept {
  osDelay(ms);
}

std::uint32_t Clock::now_ms() noexcept {
  return osKernelGetTickCount();
}

}  // namespace midismith::os

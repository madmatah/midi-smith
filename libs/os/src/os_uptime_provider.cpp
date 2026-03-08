#include "os/os_uptime_provider.hpp"

#include "cmsis_os2.h"

namespace midismith::os {

std::uint32_t OsUptimeProvider::GetUptimeMs() const noexcept {
  const std::uint32_t tick_freq = osKernelGetTickFreq();
  if (tick_freq == 0) {
    return 0;
  }
  return static_cast<std::uint32_t>(static_cast<std::uint64_t>(osKernelGetTickCount()) * 1000u /
                                    tick_freq);
}

}  // namespace midismith::os

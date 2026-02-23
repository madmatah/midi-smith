#include "bsp/rtt_logger.hpp"

#include "SEGGER_RTT.h"

namespace midismith::common::bsp {

RttLogger::RttLogger() noexcept {
  SEGGER_RTT_Init();
}

void RttLogger::vlogf(midismith::logging::Level level, const char* fmt,
                      std::va_list* args) noexcept {
  (void) level;
  (void) SEGGER_RTT_vprintf(0u, fmt, args);
}

}  // namespace midismith::common::bsp

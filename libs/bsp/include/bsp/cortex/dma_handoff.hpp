#pragma once

#include "stm32h7xx_hal.h"

namespace midismith::bsp::cortex {

inline void OrderBufferWritesBeforeStartingDma() noexcept {
  __DSB();
}

}  // namespace midismith::bsp::cortex

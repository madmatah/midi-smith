#pragma once

#include "stm32h7xx_hal.h"

namespace midismith::bsp::cortex {

inline void EnsureBufferWritesLandBeforeStartingDma() noexcept {
  __DSB();
}

}  // namespace midismith::bsp::cortex

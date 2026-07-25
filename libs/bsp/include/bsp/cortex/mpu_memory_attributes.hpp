#pragma once

#include <cstdint>

#include "stm32h7xx_hal.h"

namespace midismith::bsp::cortex {

inline constexpr std::uint8_t kTypeExtensionForNormalNonCacheable = MPU_TEX_LEVEL1;

static_assert(kTypeExtensionForNormalNonCacheable != MPU_TEX_LEVEL0,
              "a region left at TEX level 0 while non-cacheable and non-bufferable is not Normal "
              "memory but Strongly-ordered, where every unaligned access raises a UsageFault: an "
              "optimiser that merges byte loads into one word load then faults on buffers the "
              "region was meant to make merely uncached");

}  // namespace midismith::bsp::cortex

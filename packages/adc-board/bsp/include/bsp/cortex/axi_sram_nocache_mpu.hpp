#pragma once

#include <cstdint>

namespace bsp::cortex {

class AxiSramNoCacheMpu {
 public:
  static void ConfigureRegion() noexcept;
  static constexpr std::uintptr_t kBaseAddress = 0x24000000UL;
  static constexpr std::size_t kRegionSizeBytes = 8U * 1024U;
};

}  // namespace bsp::cortex

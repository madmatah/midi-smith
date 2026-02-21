#pragma once

#include <cstdint>

namespace bsp::cortex {

class D3SramNoCacheMpu {
 public:
  static void ConfigureRegion() noexcept;
  static constexpr std::uintptr_t kBaseAddress = 0x38000000UL;
  static constexpr std::size_t kRegionSizeBytes = 64U * 1024U;
};

}  // namespace bsp::cortex

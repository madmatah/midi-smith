#pragma once

#include <cstddef>
#include <cstdint>

namespace bsp::cortex {

class FlashConfigNoCacheMpu {
 public:
  static void ConfigureRegion() noexcept;
  static constexpr std::uintptr_t kBaseAddress = 0x081E0000UL;
  static constexpr std::size_t kRegionSizeBytes = 128U * 1024U;
};

}  // namespace bsp::cortex

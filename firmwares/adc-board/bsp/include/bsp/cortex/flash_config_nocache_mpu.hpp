#pragma once

namespace midismith::adc_board::bsp::cortex {

class FlashConfigNoCacheMpu {
 public:
  static void ConfigureRegion() noexcept;
};

}  // namespace midismith::adc_board::bsp::cortex

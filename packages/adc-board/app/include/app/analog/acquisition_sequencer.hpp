#pragma once

#include <cstdint>

#include "app/analog/adc_dma_control_requirements.hpp"
#include "app/analog/delay_requirements.hpp"
#include "bsp/gpio_requirements.hpp"

namespace app::analog {

class AcquisitionSequencer final {
 public:
  AcquisitionSequencer(bsp::GpioRequirements& tia_shutdown, DelayRequirements& delay,
                       AdcDmaControlRequirements& adc_dma) noexcept
      : tia_shutdown_(tia_shutdown), delay_(delay), adc_dma_(adc_dma) {}

  bool Enable(std::uint32_t settle_us) noexcept {
    tia_shutdown_.set();
    delay_.DelayUs(settle_us);
    return adc_dma_.Start();
  }

  void Disable() noexcept {
    tia_shutdown_.reset();
    adc_dma_.Stop();
  }

 private:
  bsp::GpioRequirements& tia_shutdown_;
  DelayRequirements& delay_;
  AdcDmaControlRequirements& adc_dma_;
};

}  // namespace app::analog

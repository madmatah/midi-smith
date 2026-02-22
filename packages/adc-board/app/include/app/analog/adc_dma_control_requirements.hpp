#pragma once

namespace midismith::adc_board::app::analog {

class AdcDmaControlRequirements {
 public:
  virtual ~AdcDmaControlRequirements() = default;

  virtual bool Start() noexcept = 0;
  virtual void Stop() noexcept = 0;
};

}  // namespace midismith::adc_board::app::analog

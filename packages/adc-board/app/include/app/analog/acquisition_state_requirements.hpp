#pragma once

#include "app/analog/acquisition_state.hpp"

namespace midismith::adc_board::app::analog {

class AcquisitionStateRequirements {
 public:
  virtual ~AcquisitionStateRequirements() = default;

  virtual AcquisitionState GetState() const noexcept = 0;
};

}  // namespace midismith::adc_board::app::analog

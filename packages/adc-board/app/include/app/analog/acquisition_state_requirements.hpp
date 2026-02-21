#pragma once

#include "app/analog/acquisition_state.hpp"

namespace app::analog {

class AcquisitionStateRequirements {
 public:
  virtual ~AcquisitionStateRequirements() = default;

  virtual AcquisitionState GetState() const noexcept = 0;
};

}  // namespace app::analog

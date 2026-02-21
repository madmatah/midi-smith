#pragma once

#include "app/analog/acquisition_state_requirements.hpp"

namespace app::analog {

class AcquisitionControlRequirements : public AcquisitionStateRequirements {
 public:
  virtual ~AcquisitionControlRequirements() = default;

  virtual bool RequestEnable() noexcept = 0;
  virtual bool RequestDisable() noexcept = 0;
};

}  // namespace app::analog

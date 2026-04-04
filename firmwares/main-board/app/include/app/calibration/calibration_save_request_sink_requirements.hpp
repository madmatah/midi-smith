#pragma once

#include "app/calibration/calibration_save_completion_requirements.hpp"
#include "domain/calibration/calibration_data.hpp"

namespace midismith::main_board::app::calibration {

class CalibrationSaveRequestSinkRequirements {
 public:
  virtual ~CalibrationSaveRequestSinkRequirements() = default;
  virtual void RequestSave(const domain::calibration::CalibrationData& data,
                           CalibrationSaveCompletionRequirements& on_complete) noexcept = 0;
};

}  // namespace midismith::main_board::app::calibration

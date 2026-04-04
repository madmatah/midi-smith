#pragma once

namespace midismith::main_board::app::calibration {

class CalibrationSaveCompletionRequirements {
 public:
  virtual ~CalibrationSaveCompletionRequirements() = default;
  virtual void OnSaveComplete() noexcept = 0;
};

}  // namespace midismith::main_board::app::calibration

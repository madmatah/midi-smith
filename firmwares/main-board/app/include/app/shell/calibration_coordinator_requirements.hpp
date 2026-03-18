#pragma once

#include "domain/calibration/calibration_session.hpp"

namespace midismith::main_board::app::shell {

class CalibrationCoordinatorRequirements {
 public:
  virtual ~CalibrationCoordinatorRequirements() = default;

  virtual void StartCalibration() noexcept = 0;
  virtual void FinishStrikePhase() noexcept = 0;
  virtual void ConfirmSavePartial() noexcept = 0;
  virtual void Abort() noexcept = 0;

  [[nodiscard]] virtual midismith::main_board::domain::calibration::CalibrationState state()
      const noexcept = 0;

  [[nodiscard]] virtual midismith::main_board::domain::calibration::StrikeProgress
  GetStrikeProgress() const noexcept = 0;
};

}  // namespace midismith::main_board::app::shell

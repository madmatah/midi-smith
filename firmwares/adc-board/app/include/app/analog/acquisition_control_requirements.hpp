#pragma once

#include <array>

#include "app/analog/acquisition_state_requirements.hpp"
#include "app/config/sensors.hpp"
#include "calibration/board_calibration_data.hpp"

namespace midismith::adc_board::app::analog {

class AcquisitionControlRequirements : public AcquisitionStateRequirements {
 public:
  using CalibrationArray = midismith::calibration::BoardCalibrationData<
      midismith::adc_board::app::config::sensors::kSensorCount>;

  virtual ~AcquisitionControlRequirements() = default;

  virtual bool RequestEnable() noexcept = 0;
  virtual bool RequestDisable() noexcept = 0;
  virtual bool RequestCalibrationStart() noexcept = 0;
  virtual bool RequestRestPhaseComplete() noexcept = 0;
  virtual bool RequestCalibrationDataCollection() noexcept = 0;
};

}  // namespace midismith::adc_board::app::analog

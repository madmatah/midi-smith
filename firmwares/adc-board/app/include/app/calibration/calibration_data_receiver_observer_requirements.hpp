#pragma once

#include "app/config/sensors.hpp"
#include "calibration/board_calibration_data.hpp"

namespace midismith::adc_board::app::calibration {

class CalibrationDataReceiverObserverRequirements {
 public:
  using SensorCalibrationArray = midismith::calibration::BoardCalibrationData<
      midismith::adc_board::app::config::sensors::kSensorCount>;

  virtual ~CalibrationDataReceiverObserverRequirements() = default;

  virtual void OnCalibrationDataReceived(const SensorCalibrationArray& data) noexcept = 0;
  virtual void OnCalibrationNoDataAvailable() noexcept = 0;
  virtual void OnCalibrationReceiveTimeout() noexcept = 0;
};

}  // namespace midismith::adc_board::app::calibration

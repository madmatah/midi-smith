#pragma once

#include <array>

#include "app/analog/acquisition_state_requirements.hpp"
#include "app/config/sensors.hpp"
#include "sensor-linearization/sensor_calibration.hpp"

namespace midismith::adc_board::app::analog {

class AcquisitionControlRequirements : public AcquisitionStateRequirements {
 public:
  using CalibrationArray = std::array<midismith::sensor_linearization::SensorCalibration,
                                      midismith::adc_board::app::config::sensors::kSensorCount>;

  virtual ~AcquisitionControlRequirements() = default;

  virtual bool RequestEnable() noexcept = 0;
  virtual bool RequestDisable() noexcept = 0;
  virtual bool RequestCalibrationStart() noexcept = 0;
  virtual bool RequestRestPhaseComplete() noexcept = 0;
  virtual bool RequestCalibrationDataCollection() noexcept = 0;
};

}  // namespace midismith::adc_board::app::analog

#pragma once

#include <array>
#include <cstdint>

#include "domain/config/main_board_config.hpp"
#include "sensor-linearization/sensor_calibration.hpp"

namespace midismith::main_board::app::calibration {

class CalibrationBulkDataReceiverObserverRequirements {
 public:
  using SensorCalibrationArray =
      std::array<midismith::sensor_linearization::SensorCalibration,
                 midismith::main_board::domain::config::kSensorsPerBoard>;

  virtual ~CalibrationBulkDataReceiverObserverRequirements() = default;

  virtual void OnDataReceived(std::uint8_t board_id,
                              const SensorCalibrationArray& data) noexcept = 0;
  virtual void OnReceiveTimeout(std::uint8_t board_id) noexcept = 0;
};

}  // namespace midismith::main_board::app::calibration

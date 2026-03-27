#pragma once

#include <array>
#include <cstdint>

#include "calibration/board_calibration_data.hpp"
#include "domain/config/main_board_config.hpp"

namespace midismith::main_board::app::calibration {

class CalibrationBulkDataReceiverObserverRequirements {
 public:
  using SensorCalibrationArray = midismith::calibration::BoardCalibrationData<
      midismith::main_board::domain::config::kSensorsPerBoard>;

  virtual ~CalibrationBulkDataReceiverObserverRequirements() = default;

  virtual void OnDataReceived(std::uint8_t board_id,
                              const SensorCalibrationArray& data) noexcept = 0;
  virtual void OnReceiveTimeout(std::uint8_t board_id) noexcept = 0;
};

}  // namespace midismith::main_board::app::calibration

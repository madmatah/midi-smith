#pragma once

#include <array>

#include "domain/config/main_board_config.hpp"
#include "sensor-linearization/sensor_calibration.hpp"

namespace midismith::main_board::domain::calibration {

struct CalibrationData {
  std::array<std::array<midismith::sensor_linearization::SensorCalibration,
                        midismith::main_board::domain::config::kSensorsPerBoard>,
             midismith::main_board::domain::config::kMaxBoardCount>
      sensor_calibrations;
  std::array<bool, midismith::main_board::domain::config::kMaxBoardCount> board_data_valid;
};

}  // namespace midismith::main_board::domain::calibration

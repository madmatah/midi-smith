#pragma once

#include <array>

#include "calibration/board_calibration_data.hpp"
#include "domain/config/main_board_config.hpp"

namespace midismith::main_board::domain::calibration {

struct CalibrationData {
  std::array<midismith::calibration::BoardCalibrationData<
                 midismith::main_board::domain::config::kSensorsPerBoard>,
             midismith::main_board::domain::config::kMaxBoardCount>
      sensor_calibrations;
  std::array<bool, midismith::main_board::domain::config::kMaxBoardCount> board_data_valid;
};

}  // namespace midismith::main_board::domain::calibration

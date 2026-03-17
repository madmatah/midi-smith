#pragma once

#include <cstdint>

namespace midismith::adc_board::app::analog {

enum class AcquisitionCommand : std::uint8_t {
  kEnable = 0,
  kDisable = 1,
  kCalibrationStart = 2,
  kRestPhaseComplete = 3,
  kCollectCalibrationData = 4,
};

}  // namespace midismith::adc_board::app::analog

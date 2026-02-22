#pragma once

#include <cstdint>

namespace midismith::adc_board::app::analog {

enum class AcquisitionCommand : std::uint8_t {
  kEnable = 0,
  kDisable = 1,
};

}  // namespace midismith::adc_board::app::analog

#pragma once

#include <cstdint>

namespace midismith::adc_board::app::analog {

enum class AcquisitionState : std::uint8_t {
  kDisabled = 0,
  kEnabled = 1,
};

}  // namespace midismith::adc_board::app::analog

#pragma once

#include <cstdint>

namespace midismith::main_board::domain::adc {

enum class AdcBoardState : std::uint8_t {
  kElectricallyOff,
  kElectricallyOn,
  kUnresponsive,
  kReachable,
};

}  // namespace midismith::main_board::domain::adc

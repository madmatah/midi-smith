#pragma once

#include <cstdint>

namespace midismith::main_board::domain::adc {

class AdcBoardPowerSwitchRequirements {
 public:
  virtual ~AdcBoardPowerSwitchRequirements() = default;
  virtual void PowerOn(std::uint8_t peer_id) noexcept = 0;
  virtual void PowerOff(std::uint8_t peer_id) noexcept = 0;
};

}  // namespace midismith::main_board::domain::adc

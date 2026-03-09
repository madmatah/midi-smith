#pragma once

#include <cstdint>

#include "domain/adc/adc_board_state.hpp"

namespace midismith::main_board::app::shell {

class AdcBoardsControlRequirements {
 public:
  virtual ~AdcBoardsControlRequirements() = default;
  virtual void StartPowerSequence() noexcept = 0;
  virtual void StopAll() noexcept = 0;
  virtual void PowerOn(std::uint8_t peer_id) noexcept = 0;
  virtual void PowerOff(std::uint8_t peer_id) noexcept = 0;
  [[nodiscard]] virtual midismith::main_board::domain::adc::AdcBoardState board_state(
      std::uint8_t peer_id) const noexcept = 0;
};

}  // namespace midismith::main_board::app::shell

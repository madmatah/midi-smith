#pragma once

#include <atomic>

#include "domain/adc/adc_board_state.hpp"

namespace midismith::main_board::domain::adc {

class AdcBoardController {
 public:
  AdcBoardController() noexcept = default;

  void PowerOn() noexcept;
  void PowerOff() noexcept;
  void MarkUnresponsive() noexcept;
  void OnReachable() noexcept;
  void OnLost() noexcept;

  [[nodiscard]] AdcBoardState state() const noexcept;

 private:
  std::atomic<AdcBoardState> state_{AdcBoardState::kElectricallyOff};
};

}  // namespace midismith::main_board::domain::adc

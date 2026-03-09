#pragma once

#include <cstdint>

namespace midismith::main_board::app::adc {

enum class AdcBoardState : std::uint8_t {
  kElectricallyOff,
  kElectricallyOn,
  kUnresponsive,
  kInitializing,
  kReady,
  kAcquiring,
};

[[nodiscard]] inline bool IsConnected(AdcBoardState state) noexcept {
  return state == AdcBoardState::kInitializing || state == AdcBoardState::kReady ||
         state == AdcBoardState::kAcquiring;
}

}  // namespace midismith::main_board::app::adc

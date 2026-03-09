#include "domain/adc/adc_board_controller.hpp"

namespace midismith::main_board::domain::adc {

void AdcBoardController::PowerOn() noexcept {
  if (state_.load(std::memory_order_relaxed) == AdcBoardState::kElectricallyOff) {
    state_.store(AdcBoardState::kElectricallyOn, std::memory_order_relaxed);
  }
}

void AdcBoardController::PowerOff() noexcept {
  state_.store(AdcBoardState::kElectricallyOff, std::memory_order_relaxed);
}

void AdcBoardController::MarkUnresponsive() noexcept {
  if (state_.load(std::memory_order_relaxed) == AdcBoardState::kElectricallyOn) {
    state_.store(AdcBoardState::kUnresponsive, std::memory_order_relaxed);
  }
}

void AdcBoardController::OnReachable() noexcept {
  const auto current = state_.load(std::memory_order_relaxed);
  if (current == AdcBoardState::kElectricallyOn || current == AdcBoardState::kUnresponsive) {
    state_.store(AdcBoardState::kReachable, std::memory_order_relaxed);
  }
}

void AdcBoardController::OnLost() noexcept {
  if (state_.load(std::memory_order_relaxed) == AdcBoardState::kReachable) {
    state_.store(AdcBoardState::kElectricallyOn, std::memory_order_relaxed);
  }
}

AdcBoardState AdcBoardController::state() const noexcept {
  return state_.load(std::memory_order_relaxed);
}

}  // namespace midismith::main_board::domain::adc

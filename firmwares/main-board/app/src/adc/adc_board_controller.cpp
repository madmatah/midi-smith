#include "app/adc/adc_board_controller.hpp"

#include "protocol/messages.hpp"

namespace midismith::main_board::app::adc {

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

void AdcBoardController::OnPeerHealthy(midismith::protocol::DeviceState device_state) noexcept {
  const auto current = state_.load(std::memory_order_relaxed);
  const auto target = MapDeviceState(device_state);
  const bool is_first_contact =
      current == AdcBoardState::kElectricallyOn || current == AdcBoardState::kUnresponsive;

  if (is_first_contact) {
    state_.store(target, std::memory_order_relaxed);
    sender_.SendHeartbeat(midismith::protocol::DeviceState::kRunning);
    if (target == AdcBoardState::kReady) {
      sender_.SendStartAdc(peer_id_);
    }
    return;
  }

  if (!IsConnected(current)) {
    return;
  }

  if (target != current) {
    state_.store(target, std::memory_order_relaxed);
    if (target == AdcBoardState::kReady && current != AdcBoardState::kReady) {
      sender_.SendStartAdc(peer_id_);
    }
  }
}

void AdcBoardController::OnLost() noexcept {
  if (IsConnected(state_.load(std::memory_order_relaxed))) {
    state_.store(AdcBoardState::kElectricallyOn, std::memory_order_relaxed);
  }
}

AdcBoardState AdcBoardController::state() const noexcept {
  return state_.load(std::memory_order_relaxed);
}

AdcBoardState AdcBoardController::MapDeviceState(
    midismith::protocol::DeviceState device_state) const noexcept {
  switch (device_state) {
    case midismith::protocol::DeviceState::kRunning:
      return AdcBoardState::kAcquiring;
    case midismith::protocol::DeviceState::kReady:
      return AdcBoardState::kReady;
    default:
      return AdcBoardState::kInitializing;
  }
}

}  // namespace midismith::main_board::app::adc

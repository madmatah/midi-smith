#pragma once

#include <atomic>
#include <cstdint>

#include "app/adc/adc_board_state.hpp"
#include "app/messaging/main_board_message_sender_requirements.hpp"
#include "protocol/messages.hpp"

namespace midismith::main_board::app::adc {

class AdcBoardController {
 public:
  AdcBoardController(messaging::MainBoardMessageSenderRequirements& sender,
                     std::uint8_t peer_id) noexcept
      : sender_(sender), peer_id_(peer_id) {}

  void PowerOn() noexcept;
  void PowerOff() noexcept;
  void MarkUnresponsive() noexcept;
  void OnPeerHealthy(midismith::protocol::DeviceState device_state) noexcept;
  void OnLost() noexcept;

  [[nodiscard]] AdcBoardState state() const noexcept;

 private:
  [[nodiscard]] AdcBoardState MapDeviceState(
      midismith::protocol::DeviceState device_state) const noexcept;

  messaging::MainBoardMessageSenderRequirements& sender_;
  std::uint8_t peer_id_;
  std::atomic<AdcBoardState> state_{AdcBoardState::kElectricallyOff};
};

}  // namespace midismith::main_board::app::adc

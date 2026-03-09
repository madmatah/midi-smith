#pragma once

#include <atomic>

#include "app/adc/adc_board_state.hpp"
#include "app/messaging/main_board_message_sender_requirements.hpp"

namespace midismith::main_board::app::adc {

class AdcBoardController {
 public:
  explicit AdcBoardController(messaging::MainBoardMessageSenderRequirements& sender) noexcept
      : sender_(sender) {}

  void PowerOn() noexcept;
  void PowerOff() noexcept;
  void MarkUnresponsive() noexcept;
  void OnReachable() noexcept;
  void OnLost() noexcept;

  [[nodiscard]] AdcBoardState state() const noexcept;

 private:
  messaging::MainBoardMessageSenderRequirements& sender_;
  std::atomic<AdcBoardState> state_{AdcBoardState::kElectricallyOff};
};

}  // namespace midismith::main_board::app::adc

#pragma once

#include "app/messaging/main_board_message_sender_requirements.hpp"
#include "bsp-types/can/fdcan_transceiver_requirements.hpp"
#include "protocol/messages.hpp"

namespace midismith::main_board::app::messaging {

class MainBoardCanMessageSender final : public MainBoardMessageSenderRequirements {
 public:
  explicit MainBoardCanMessageSender(bsp::can::FdcanTransceiverRequirements& transceiver) noexcept;

  bool SendHeartbeat(protocol::DeviceState device_state) noexcept override;
  bool SendStartAdc(std::uint8_t target_node_id) noexcept override;
  bool SendStopAdc(std::uint8_t target_node_id) noexcept override;
  bool SendStartCalibration(std::uint8_t target_node_id,
                            protocol::CalibMode mode) noexcept override;

 private:
  bsp::can::FdcanTransceiverRequirements& transceiver_;
};

}  // namespace midismith::main_board::app::messaging

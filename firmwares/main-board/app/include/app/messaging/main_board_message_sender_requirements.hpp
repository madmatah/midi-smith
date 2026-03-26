#pragma once

#include <cstdint>

#include "protocol/messages.hpp"

namespace midismith::main_board::app::messaging {

class MainBoardMessageSenderRequirements {
 public:
  virtual ~MainBoardMessageSenderRequirements() = default;

  virtual bool SendHeartbeat(protocol::DeviceState device_state) noexcept = 0;
  virtual bool SendStartAdc(std::uint8_t target_node_id) noexcept = 0;
  virtual bool SendStopAdc(std::uint8_t target_node_id) noexcept = 0;
  virtual bool SendStartCalibration(std::uint8_t target_node_id,
                                    protocol::CalibMode mode) noexcept = 0;
  virtual bool SendDumpRequest(std::uint8_t target_node_id) noexcept = 0;
  virtual bool SendCalibrationAck(std::uint8_t target_node_id, std::uint8_t ack_index,
                                  protocol::DataSegmentAckStatus status) noexcept = 0;
};

}  // namespace midismith::main_board::app::messaging

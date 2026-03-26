#pragma once

#include <cstdint>

#include "app/messaging/adc_board_message_sender_requirements.hpp"
#include "bsp-types/can/fdcan_transceiver_requirements.hpp"

namespace midismith::adc_board::app::messaging {

class AdcBoardCanMessageSender final : public AdcBoardMessageSenderRequirements {
 public:
  explicit AdcBoardCanMessageSender(bsp::can::FdcanTransceiverRequirements& transceiver,
                                    const std::uint8_t& board_id) noexcept;

  bool SendNoteOn(std::uint8_t sensor_id, std::uint8_t velocity) noexcept override;
  bool SendNoteOff(std::uint8_t sensor_id, std::uint8_t velocity) noexcept override;
  bool SendHeartbeat(protocol::DeviceState device_state) noexcept override;
  bool SendCalibrationDataSegment(
      std::uint8_t seq_index, std::uint8_t total_packets,
      const std::array<std::uint8_t, protocol::CalibrationDataSegment::kPayloadSizeBytes>&
          payload) noexcept override;

 private:
  bsp::can::FdcanTransceiverRequirements& transceiver_;
  const std::uint8_t& board_id_;
};

}  // namespace midismith::adc_board::app::messaging

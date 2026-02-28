#include "app/messaging/adc_board_can_message_sender.hpp"

#include <array>
#include <span>

#include "bsp-types/can/fdcan_frame.hpp"
#include "protocol-can/can_mapper.hpp"
#include "protocol/builders.hpp"

namespace midismith::adc_board::app::messaging {

AdcBoardCanMessageSender::AdcBoardCanMessageSender(
    bsp::can::FdcanTransceiverRequirements& transceiver, const std::uint8_t& board_id) noexcept
    : transceiver_(transceiver), board_id_(board_id) {}

bool AdcBoardCanMessageSender::SendNoteOn(std::uint8_t sensor_id, std::uint8_t velocity) noexcept {
  const auto [header, event] =
      protocol::AdcMessageBuilder(board_id_).BuildNoteOn(sensor_id, velocity);
  const auto can_id = protocol_can::CanIdentifierMapper::EncodeId(header);

  std::array<std::uint8_t, bsp::can::kClassicCanMaxDataBytes> buffer{};
  const auto bytes_written = event.Serialize(std::span(buffer));
  if (!bytes_written) return false;

  return transceiver_.Transmit(bsp::can::FdcanFrame{
      .identifier = can_id,
      .data_length_bytes = *bytes_written,
      .data = buffer,
  });
}

bool AdcBoardCanMessageSender::SendNoteOff(std::uint8_t sensor_id, std::uint8_t velocity) noexcept {
  const auto [header, event] =
      protocol::AdcMessageBuilder(board_id_).BuildNoteOff(sensor_id, velocity);
  const auto can_id = protocol_can::CanIdentifierMapper::EncodeId(header);

  std::array<std::uint8_t, bsp::can::kClassicCanMaxDataBytes> buffer{};
  const auto bytes_written = event.Serialize(std::span(buffer));
  if (!bytes_written) return false;

  return transceiver_.Transmit(bsp::can::FdcanFrame{
      .identifier = can_id,
      .data_length_bytes = *bytes_written,
      .data = buffer,
  });
}

bool AdcBoardCanMessageSender::SendHeartbeat(protocol::DeviceState device_state) noexcept {
  const auto [header, heartbeat] =
      protocol::AdcMessageBuilder(board_id_).BuildHeartbeat(device_state);
  const auto can_id = protocol_can::CanIdentifierMapper::EncodeId(header);

  std::array<std::uint8_t, bsp::can::kClassicCanMaxDataBytes> buffer{};
  const auto bytes_written = heartbeat.Serialize(std::span(buffer));
  if (!bytes_written) return false;

  return transceiver_.Transmit(bsp::can::FdcanFrame{
      .identifier = can_id,
      .data_length_bytes = *bytes_written,
      .data = buffer,
  });
}

}  // namespace midismith::adc_board::app::messaging

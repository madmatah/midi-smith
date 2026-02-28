#include "app/messaging/main_board_can_message_sender.hpp"

#include <array>
#include <span>

#include "bsp-types/can/fdcan_frame.hpp"
#include "protocol-can/can_mapper.hpp"
#include "protocol/builders.hpp"

namespace midismith::main_board::app::messaging {

MainBoardCanMessageSender::MainBoardCanMessageSender(
    bsp::can::FdcanTransceiverRequirements& transceiver) noexcept
    : transceiver_(transceiver) {}

bool MainBoardCanMessageSender::SendStartAdc(std::uint8_t target_node_id) noexcept {
  const auto [header, command] = protocol::MainBoardMessageBuilder().BuildStartAdc(target_node_id);
  const auto can_id = protocol_can::CanIdentifierMapper::EncodeId(header);

  std::array<std::uint8_t, bsp::can::kClassicCanMaxDataBytes> buffer{};
  const auto bytes_written = protocol::Serialize(command, std::span(buffer));
  if (!bytes_written) return false;

  return transceiver_.Transmit(bsp::can::FdcanFrame{
      .identifier = can_id,
      .data_length_bytes = *bytes_written,
      .data = buffer,
  });
}

bool MainBoardCanMessageSender::SendStopAdc(std::uint8_t target_node_id) noexcept {
  const auto [header, command] = protocol::MainBoardMessageBuilder().BuildStopAdc(target_node_id);
  const auto can_id = protocol_can::CanIdentifierMapper::EncodeId(header);

  std::array<std::uint8_t, bsp::can::kClassicCanMaxDataBytes> buffer{};
  const auto bytes_written = protocol::Serialize(command, std::span(buffer));
  if (!bytes_written) return false;

  return transceiver_.Transmit(bsp::can::FdcanFrame{
      .identifier = can_id,
      .data_length_bytes = *bytes_written,
      .data = buffer,
  });
}

bool MainBoardCanMessageSender::SendStartCalibration(std::uint8_t target_node_id,
                                                     protocol::CalibMode mode) noexcept {
  const auto [header, command] =
      protocol::MainBoardMessageBuilder().BuildStartCalibration(target_node_id, mode);
  const auto can_id = protocol_can::CanIdentifierMapper::EncodeId(header);

  std::array<std::uint8_t, bsp::can::kClassicCanMaxDataBytes> buffer{};
  const auto bytes_written = protocol::Serialize(command, std::span(buffer));
  if (!bytes_written) return false;

  return transceiver_.Transmit(bsp::can::FdcanFrame{
      .identifier = can_id,
      .data_length_bytes = *bytes_written,
      .data = buffer,
  });
}

}  // namespace midismith::main_board::app::messaging

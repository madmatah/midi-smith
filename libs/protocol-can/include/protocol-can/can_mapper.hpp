#pragma once

#include <cstdint>
#include <optional>

#include "protocol/topology.hpp"
#include "protocol/transport_header.hpp"

namespace midismith::protocol_can {

class CanIdentifierMapper {
 public:
  [[nodiscard]] static constexpr std::uint16_t EncodeId(const protocol::TransportHeader& header);

  [[nodiscard]] static constexpr std::optional<protocol::TransportHeader> DecodeId(
      std::uint16_t can_id);

 private:
  static constexpr std::uint16_t kElevenBitIdentifierMaxValue = 0x7FF;
  static constexpr std::uint8_t kNodeIdentifierBitWidth = 4;
  static constexpr std::uint16_t kNodeIdentifierBitMask = 0x0F;

  static constexpr std::uint8_t kRealTimeNoteEventFunctionCode = 0x01;
  static constexpr std::uint8_t kControlBroadcastFunctionCode = 0x10;
  static constexpr std::uint8_t kControlUnicastFunctionCode = 0x11;
  static constexpr std::uint8_t kBulkDataSegmentFunctionCode = 0x21;
  static constexpr std::uint8_t kSystemHeartbeatFunctionCode = 0x71;

  static constexpr std::uint8_t kMainBoardNodeId =
      static_cast<std::uint8_t>(protocol::NodeRole::kMainBoard);

  static constexpr std::uint16_t AssembleCanId(std::uint8_t function_code, std::uint8_t node_id);
};

constexpr std::uint16_t CanIdentifierMapper::AssembleCanId(std::uint8_t function_code,
                                                           std::uint8_t node_id) {
  return static_cast<std::uint16_t>(
      (static_cast<std::uint16_t>(function_code) << kNodeIdentifierBitWidth) | node_id);
}

constexpr std::uint16_t CanIdentifierMapper::EncodeId(const protocol::TransportHeader& header) {
  using protocol::MessageCategory;

  if (header.category == MessageCategory::kRealTime) {
    return AssembleCanId(kRealTimeNoteEventFunctionCode, header.source_node_id);
  }

  if (header.category == MessageCategory::kControl) {
    const bool is_broadcast = (header.destination_node_id == kMainBoardNodeId);
    if (is_broadcast) {
      return AssembleCanId(kControlBroadcastFunctionCode, kMainBoardNodeId);
    }
    return AssembleCanId(kControlUnicastFunctionCode, header.destination_node_id);
  }

  if (header.category == MessageCategory::kBulkData) {
    const std::uint8_t adc_node_id = (header.source_node_id != kMainBoardNodeId)
                                         ? header.source_node_id
                                         : header.destination_node_id;
    return AssembleCanId(kBulkDataSegmentFunctionCode, adc_node_id);
  }

  return AssembleCanId(kSystemHeartbeatFunctionCode, header.source_node_id);
}

constexpr std::optional<protocol::TransportHeader> CanIdentifierMapper::DecodeId(
    std::uint16_t can_id) {
  using protocol::MessageCategory;
  using protocol::MessageType;
  using protocol::TransportHeader;

  if (can_id > kElevenBitIdentifierMaxValue) {
    return std::nullopt;
  }

  const auto function_code = static_cast<std::uint8_t>(can_id >> kNodeIdentifierBitWidth);
  const auto node_id = static_cast<std::uint8_t>(can_id & kNodeIdentifierBitMask);

  if (function_code == kRealTimeNoteEventFunctionCode) {
    return TransportHeader::ReconstructFromTransport(
        MessageCategory::kRealTime, MessageType::kNoteEvent, node_id, kMainBoardNodeId);
  }

  if (function_code == kControlBroadcastFunctionCode && node_id == kMainBoardNodeId) {
    return TransportHeader::ReconstructFromTransport(
        MessageCategory::kControl, MessageType::kCommand, kMainBoardNodeId, kMainBoardNodeId);
  }

  if (function_code == kControlUnicastFunctionCode) {
    return TransportHeader::ReconstructFromTransport(
        MessageCategory::kControl, MessageType::kCommand, kMainBoardNodeId, node_id);
  }

  if (function_code == kBulkDataSegmentFunctionCode) {
    return TransportHeader::ReconstructFromTransport(
        MessageCategory::kBulkData, MessageType::kDataSegment, node_id, kMainBoardNodeId);
  }

  if (function_code == kSystemHeartbeatFunctionCode) {
    return TransportHeader::ReconstructFromTransport(
        MessageCategory::kSystem, MessageType::kHeartbeat, node_id, kMainBoardNodeId);
  }

  return std::nullopt;
}

}  // namespace midismith::protocol_can

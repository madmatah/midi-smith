#pragma once

#include <cstdint>
#include <optional>
#include <variant>

#include "protocol/topology.hpp"
#include "protocol/transport_header.hpp"

namespace midismith::protocol_can {

using DecodedTransportHeader =
    std::variant<protocol::UnicastTransportHeader, protocol::BroadcastTransportHeader>;

class CanIdentifierMapper {
 public:
  [[nodiscard]] static constexpr std::uint16_t EncodeId(
      const protocol::UnicastTransportHeader& header);

  [[nodiscard]] static constexpr std::uint16_t EncodeId(
      const protocol::BroadcastTransportHeader& header);

  [[nodiscard]] static constexpr std::optional<DecodedTransportHeader> DecodeId(
      std::uint16_t can_id);

 private:
  static constexpr std::uint16_t kElevenBitIdentifierMaxValue = 0x7FF;
  static constexpr std::uint8_t kNodeIdentifierBitWidth = 4;
  static constexpr std::uint16_t kNodeIdentifierBitMask = 0x0F;

  static constexpr std::uint8_t kRealTimeSensorEventFunctionCode = 0x01;
  static constexpr std::uint8_t kControlBroadcastFunctionCode = 0x10;
  static constexpr std::uint8_t kControlUnicastFunctionCode = 0x11;
  static constexpr std::uint8_t kBulkDataSegmentFunctionCode = 0x21;
  static constexpr std::uint8_t kSystemHeartbeatFunctionCode = 0x71;

  static constexpr std::uint8_t kMainBoardNodeId = protocol::kMainBoardNodeId;

  static constexpr std::uint16_t AssembleCanId(std::uint8_t function_code, std::uint8_t node_id);
};

constexpr std::uint16_t CanIdentifierMapper::AssembleCanId(std::uint8_t function_code,
                                                           std::uint8_t node_id) {
  return static_cast<std::uint16_t>(
      (static_cast<std::uint16_t>(function_code) << kNodeIdentifierBitWidth) | node_id);
}

constexpr std::uint16_t CanIdentifierMapper::EncodeId(
    const protocol::UnicastTransportHeader& header) {
  using protocol::MessageCategory;

  if (header.category == MessageCategory::kRealTime) {
    return AssembleCanId(kRealTimeSensorEventFunctionCode, header.source_node_id);
  }

  if (header.category == MessageCategory::kControl) {
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

constexpr std::uint16_t CanIdentifierMapper::EncodeId(
    const protocol::BroadcastTransportHeader& header) {
  using protocol::MessageCategory;

  if (header.category == MessageCategory::kControl) {
    return AssembleCanId(kControlBroadcastFunctionCode, kMainBoardNodeId);
  }

  return AssembleCanId(kSystemHeartbeatFunctionCode, header.source_node_id);
}

constexpr std::optional<DecodedTransportHeader> CanIdentifierMapper::DecodeId(
    std::uint16_t can_id) {
  using protocol::BroadcastTransportHeader;
  using protocol::MessageCategory;
  using protocol::MessageType;
  using protocol::UnicastTransportHeader;

  if (can_id > kElevenBitIdentifierMaxValue) {
    return std::nullopt;
  }

  const auto function_code = static_cast<std::uint8_t>(can_id >> kNodeIdentifierBitWidth);
  const auto node_id = static_cast<std::uint8_t>(can_id & kNodeIdentifierBitMask);

  if (function_code == kRealTimeSensorEventFunctionCode) {
    return UnicastTransportHeader::Make(MessageCategory::kRealTime, MessageType::kSensorEvent,
                                        node_id, kMainBoardNodeId);
  }

  if (function_code == kControlBroadcastFunctionCode && node_id == kMainBoardNodeId) {
    return BroadcastTransportHeader::Make(MessageCategory::kControl, MessageType::kCommand,
                                          kMainBoardNodeId);
  }

  if (function_code == kControlUnicastFunctionCode) {
    return UnicastTransportHeader::Make(MessageCategory::kControl, MessageType::kCommand,
                                        kMainBoardNodeId, node_id);
  }

  if (function_code == kBulkDataSegmentFunctionCode) {
    return UnicastTransportHeader::Make(MessageCategory::kBulkData, MessageType::kDataSegment,
                                        node_id, kMainBoardNodeId);
  }

  if (function_code == kSystemHeartbeatFunctionCode) {
    if (node_id == kMainBoardNodeId) {
      return BroadcastTransportHeader::Make(MessageCategory::kSystem, MessageType::kHeartbeat,
                                            kMainBoardNodeId);
    }
    return UnicastTransportHeader::Make(MessageCategory::kSystem, MessageType::kHeartbeat, node_id,
                                        kMainBoardNodeId);
  }

  return std::nullopt;
}

}  // namespace midismith::protocol_can

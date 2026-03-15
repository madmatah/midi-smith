#pragma once

#include <array>
#include <cstdint>
#include <utility>

#include "protocol/messages.hpp"
#include "protocol/transport_header.hpp"

namespace midismith::protocol {

class AdcMessageBuilder {
 public:
  explicit constexpr AdcMessageBuilder(std::uint8_t my_node_id) : node_id_(my_node_id) {}

  [[nodiscard]] constexpr std::pair<UnicastTransportHeader, SensorEvent> BuildNoteOn(
      std::uint8_t sensor_id, std::uint8_t velocity) const {
    return {UnicastTransportHeader(MessageCategory::kRealTime, MessageType::kSensorEvent, node_id_,
                                   kMainBoardNodeId),
            SensorEvent{
                .type = SensorEventType::kNoteOn, .sensor_id = sensor_id, .velocity = velocity}};
  }

  [[nodiscard]] constexpr std::pair<UnicastTransportHeader, SensorEvent> BuildNoteOff(
      std::uint8_t sensor_id, std::uint8_t velocity) const {
    return {UnicastTransportHeader(MessageCategory::kRealTime, MessageType::kSensorEvent, node_id_,
                                   kMainBoardNodeId),
            SensorEvent{
                .type = SensorEventType::kNoteOff, .sensor_id = sensor_id, .velocity = velocity}};
  }

  [[nodiscard]] constexpr std::pair<UnicastTransportHeader, Heartbeat> BuildHeartbeat(
      DeviceState device_state) const {
    return {UnicastTransportHeader(MessageCategory::kSystem, MessageType::kHeartbeat, node_id_,
                                   kMainBoardNodeId),
            Heartbeat{.device_state = device_state}};
  }

  [[nodiscard]] constexpr std::pair<UnicastTransportHeader, CalibrationDataSegment>
  BuildCalibrationDataSegment(
      std::uint8_t seq_index, std::uint8_t total_packets,
      std::array<std::uint8_t, CalibrationDataSegment::kPayloadSizeBytes> payload) const {
    return {UnicastTransportHeader(MessageCategory::kBulkData, MessageType::kDataSegment, node_id_,
                                   kMainBoardNodeId),
            CalibrationDataSegment{
                .seq_index = seq_index, .total_packets = total_packets, .payload = payload}};
  }

 private:
  std::uint8_t node_id_;
};

class MainBoardMessageBuilder {
 public:
  explicit constexpr MainBoardMessageBuilder() = default;

  [[nodiscard]] constexpr std::pair<BroadcastTransportHeader, Heartbeat> BuildHeartbeat(
      DeviceState device_state) const {
    return {BroadcastTransportHeader(MessageCategory::kSystem, MessageType::kHeartbeat,
                                     kMainBoardNodeId),
            Heartbeat{.device_state = device_state}};
  }

  [[nodiscard]] constexpr std::pair<UnicastTransportHeader, Command> BuildStartAdc(
      std::uint8_t target_node_id) const {
    return {UnicastTransportHeader(MessageCategory::kControl, MessageType::kCommand,
                                   kMainBoardNodeId, target_node_id),
            Command(AdcStart{})};
  }

  [[nodiscard]] constexpr std::pair<UnicastTransportHeader, Command> BuildStopAdc(
      std::uint8_t target_node_id) const {
    return {UnicastTransportHeader(MessageCategory::kControl, MessageType::kCommand,
                                   kMainBoardNodeId, target_node_id),
            Command(AdcStop{})};
  }

  [[nodiscard]] constexpr std::pair<UnicastTransportHeader, Command> BuildStartCalibration(
      std::uint8_t target_node_id, CalibMode mode) const {
    return {UnicastTransportHeader(MessageCategory::kControl, MessageType::kCommand,
                                   kMainBoardNodeId, target_node_id),
            Command(CalibStart{.mode = mode})};
  }

  [[nodiscard]] constexpr std::pair<UnicastTransportHeader, DataSegmentAck> BuildDataSegmentAck(
      std::uint8_t target_node_id, std::uint8_t ack_index, DataSegmentAckStatus status) const {
    return {UnicastTransportHeader(MessageCategory::kBulkData, MessageType::kDataSegmentAck,
                                   kMainBoardNodeId, target_node_id),
            DataSegmentAck{.ack_index = ack_index, .status = status}};
  }
};

}  // namespace midismith::protocol

#pragma once

#include <cstdint>
#include <utility>

#include "protocol/messages.hpp"
#include "protocol/transport_header.hpp"

namespace midismith::protocol {

class AdcMessageBuilder {
 public:
  explicit constexpr AdcMessageBuilder(std::uint8_t my_node_id) : node_id_(my_node_id) {}

  [[nodiscard]] constexpr std::pair<TransportHeader, SensorEvent> BuildNoteOn(
      std::uint8_t sensor_id, std::uint8_t velocity) const {
    return {TransportHeader(MessageCategory::kRealTime, MessageType::kSensorEvent, node_id_, 0),
            SensorEvent{
                .type = SensorEventType::kNoteOn, .sensor_id = sensor_id, .velocity = velocity}};
  }

  [[nodiscard]] constexpr std::pair<TransportHeader, SensorEvent> BuildNoteOff(
      std::uint8_t sensor_id) const {
    return {TransportHeader(MessageCategory::kRealTime, MessageType::kSensorEvent, node_id_, 0),
            SensorEvent{.type = SensorEventType::kNoteOff, .sensor_id = sensor_id, .velocity = 0}};
  }

  [[nodiscard]] constexpr std::pair<TransportHeader, Heartbeat> BuildHeartbeat(
      std::uint8_t device_state) const {
    return {TransportHeader(MessageCategory::kSystem, MessageType::kHeartbeat, node_id_, 0),
            Heartbeat{.device_state = device_state}};
  }

 private:
  std::uint8_t node_id_;
};

class MainBoardMessageBuilder {
 public:
  explicit constexpr MainBoardMessageBuilder() = default;

  [[nodiscard]] constexpr std::pair<TransportHeader, Command> BuildStartCalibration(
      std::uint8_t target_node_id, bool auto_mode) const {
    return {TransportHeader(MessageCategory::kControl, MessageType::kCommand,
                            static_cast<std::uint8_t>(NodeRole::kMainBoard), target_node_id),
            Command{.action_code = 0x03,
                    .parameter = auto_mode ? static_cast<std::uint8_t>(0x00)
                                           : static_cast<std::uint8_t>(0x01)}};
  }
};

}  // namespace midismith::protocol

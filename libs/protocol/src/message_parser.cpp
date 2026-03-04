#include "protocol/message_parser.hpp"

#include <utility>

#include "byte-codec/little_endian.hpp"

namespace midismith::protocol {

using byte_codec::ReadLittleEndian;

using MessageContent = std::variant<SensorEvent, Command, Heartbeat>;

static std::optional<MessageContent> DecodeContent(MessageCategory category, MessageType type,
                                                   std::span<const uint8_t> payload) {
  switch (category) {
    case MessageCategory::kRealTime:
      if (type == MessageType::kSensorEvent) {
        if (payload.size() < 3) return std::nullopt;

        const auto type_byte = ReadLittleEndian<std::uint8_t>(payload, 0);
        if (type_byte > static_cast<std::uint8_t>(SensorEventType::kNoteOn)) return std::nullopt;

        return SensorEvent{.type = static_cast<SensorEventType>(type_byte),
                           .sensor_id = ReadLittleEndian<std::uint8_t>(payload, 1),
                           .velocity = ReadLittleEndian<std::uint8_t>(payload, 2)};
      }
      break;

    case MessageCategory::kControl:
      if (type == MessageType::kCommand) {
        if (payload.empty()) return std::nullopt;

        const auto action = static_cast<CommandAction>(ReadLittleEndian<std::uint8_t>(payload, 0));
        switch (action) {
          case CommandAction::kAdcStart:
            return Command(AdcStart{});
          case CommandAction::kAdcStop:
            return Command(AdcStop{});
          case CommandAction::kCalibStart: {
            if (payload.size() < 2) return std::nullopt;
            const auto mode_byte = ReadLittleEndian<std::uint8_t>(payload, 1);
            if (mode_byte > static_cast<std::uint8_t>(CalibMode::kManual)) return std::nullopt;
            return Command(CalibStart{static_cast<CalibMode>(mode_byte)});
          }
          case CommandAction::kDumpRequest:
            return Command(DumpRequest{});
          default:
            return std::nullopt;
        }
      }
      break;

    case MessageCategory::kSystem:
      if (type == MessageType::kHeartbeat) {
        if (payload.empty()) return std::nullopt;

        auto state_byte = ReadLittleEndian<std::uint8_t>(payload, 0);
        if (state_byte > static_cast<std::uint8_t>(DeviceState::kError)) return std::nullopt;

        return Heartbeat{static_cast<DeviceState>(state_byte)};
      }
      break;

    case MessageCategory::kBulkData:
    default:
      break;
  }

  return std::nullopt;
}

std::optional<IncomingMessage> MessageParser::Decode(const UnicastTransportHeader& header,
                                                     std::span<const uint8_t> payload) {
  auto content = DecodeContent(header.category, header.type, payload);
  if (!content.has_value()) return std::nullopt;
  return IncomingMessage{.routing = header, .content = std::move(*content)};
}

std::optional<IncomingMessage> MessageParser::Decode(const BroadcastTransportHeader& header,
                                                     std::span<const uint8_t> payload) {
  auto content = DecodeContent(header.category, header.type, payload);
  if (!content.has_value()) return std::nullopt;
  return IncomingMessage{.routing = header, .content = std::move(*content)};
}

}  // namespace midismith::protocol

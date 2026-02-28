#include "protocol/message_parser.hpp"

#include "byte-codec/little_endian.hpp"

namespace midismith::protocol {

using byte_codec::ReadLittleEndian;

std::optional<IncomingMessage> MessageParser::Decode(const TransportHeader& header,
                                                     std::span<const uint8_t> payload) {
  switch (header.category) {
    case MessageCategory::kRealTime:
      if (header.type == MessageType::kSensorEvent) {
        if (payload.size() < 3) return std::nullopt;

        const auto type_byte = ReadLittleEndian<std::uint8_t>(payload, 0);
        if (type_byte > static_cast<std::uint8_t>(SensorEventType::kNoteOn)) return std::nullopt;

        return SensorEvent{.type = static_cast<SensorEventType>(type_byte),
                           .sensor_id = ReadLittleEndian<std::uint8_t>(payload, 1),
                           .velocity = ReadLittleEndian<std::uint8_t>(payload, 2)};
      }
      break;

    case MessageCategory::kControl:
      if (header.type == MessageType::kCommand) {
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
      if (header.type == MessageType::kHeartbeat) {
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

}  // namespace midismith::protocol

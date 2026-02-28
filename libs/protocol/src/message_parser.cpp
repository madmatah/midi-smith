#include "protocol/message_parser.hpp"

#include "byte-codec/little_endian.hpp"

namespace midismith::protocol {

using byte_codec::ReadLittleEndian;

std::optional<IncomingMessage> MessageParser::Decode(const TransportHeader& header,
                                                     std::span<const uint8_t> payload) {
  switch (header.category) {
    case MessageCategory::kRealTime:
      if (header.type == MessageType::kNoteEvent) {
        if (payload.size() < 3) return std::nullopt;

        if (ReadLittleEndian<std::uint8_t>(payload, 0) >
            static_cast<std::uint8_t>(NoteEventType::kNoteOn)) {
          return std::nullopt;
        }

        return NoteEvent{
            .type = static_cast<NoteEventType>(ReadLittleEndian<std::uint8_t>(payload, 0)),
            .note_index = ReadLittleEndian<std::uint8_t>(payload, 1),
            .velocity = ReadLittleEndian<std::uint8_t>(payload, 2)};
      }
      break;

    case MessageCategory::kControl:
      if (header.type == MessageType::kCommand) {
        if (payload.size() < 2) return std::nullopt;

        return Command{.action_code = ReadLittleEndian<std::uint8_t>(payload, 0),
                       .parameter = ReadLittleEndian<std::uint8_t>(payload, 1)};
      }
      break;

    case MessageCategory::kSystem:
      if (header.type == MessageType::kHeartbeat) {
        if (payload.empty()) return std::nullopt;

        return Heartbeat{.device_state = ReadLittleEndian<std::uint8_t>(payload, 0)};
      }
      break;

    case MessageCategory::kBulkData:
    default:
      break;
  }

  return std::nullopt;
}

}  // namespace midismith::protocol

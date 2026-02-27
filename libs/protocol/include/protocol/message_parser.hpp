#pragma once

#include <optional>
#include <span>
#include <variant>

#include "protocol/messages.hpp"
#include "protocol/transport_header.hpp"

namespace midismith::protocol {

using IncomingMessage = std::variant<std::monostate, NoteEvent, Command, Heartbeat>;

class MessageParser {
 public:
  static std::optional<IncomingMessage> Decode(const TransportHeader& header,
                                               std::span<const uint8_t> payload);
};

}  // namespace midismith::protocol

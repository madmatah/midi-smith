#pragma once

#include <optional>
#include <span>
#include <variant>

#include "protocol/messages.hpp"
#include "protocol/transport_header.hpp"

namespace midismith::protocol {

struct IncomingMessage {
  std::variant<UnicastTransportHeader, BroadcastTransportHeader> routing;
  std::variant<SensorEvent, Command, Heartbeat> content;
};

class MessageParser {
 public:
  static std::optional<IncomingMessage> Decode(const UnicastTransportHeader& header,
                                               std::span<const uint8_t> payload);

  static std::optional<IncomingMessage> Decode(const BroadcastTransportHeader& header,
                                               std::span<const uint8_t> payload);
};

}  // namespace midismith::protocol

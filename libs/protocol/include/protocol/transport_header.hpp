#pragma once

#include <cstdint>

#include "protocol/topology.hpp"

namespace midismith::protocol {

class AdcMessageBuilder;
class MainBoardMessageBuilder;

struct UnicastTransportHeader {
  MessageCategory category;
  MessageType type;
  std::uint8_t source_node_id;
  std::uint8_t destination_node_id;

  constexpr bool operator==(const UnicastTransportHeader&) const = default;

  [[nodiscard]] static constexpr UnicastTransportHeader Make(MessageCategory cat, MessageType typ,
                                                             std::uint8_t src, std::uint8_t dst) {
    return UnicastTransportHeader(cat, typ, src, dst);
  }

 private:
  friend class AdcMessageBuilder;
  friend class MainBoardMessageBuilder;
  constexpr UnicastTransportHeader(MessageCategory cat, MessageType typ, std::uint8_t src,
                                   std::uint8_t dst)
      : category(cat), type(typ), source_node_id(src), destination_node_id(dst) {}
};

struct BroadcastTransportHeader {
  MessageCategory category;
  MessageType type;
  std::uint8_t source_node_id;

  constexpr bool operator==(const BroadcastTransportHeader&) const = default;

  [[nodiscard]] static constexpr BroadcastTransportHeader Make(MessageCategory cat, MessageType typ,
                                                               std::uint8_t src) {
    return BroadcastTransportHeader(cat, typ, src);
  }

 private:
  friend class AdcMessageBuilder;
  friend class MainBoardMessageBuilder;
  constexpr BroadcastTransportHeader(MessageCategory cat, MessageType typ, std::uint8_t src)
      : category(cat), type(typ), source_node_id(src) {}
};

}  // namespace midismith::protocol

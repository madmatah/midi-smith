#pragma once

#include <cstdint>

#include "protocol/topology.hpp"

namespace midismith::protocol {

class AdcMessageBuilder;
class MainBoardMessageBuilder;

struct TransportHeader {
  MessageCategory category;
  MessageType type;
  std::uint8_t source_node_id;
  std::uint8_t destination_node_id;

  constexpr bool operator==(const TransportHeader&) const = default;

  [[nodiscard]] static constexpr TransportHeader ReconstructFromTransport(MessageCategory cat,
                                                                          MessageType typ,
                                                                          std::uint8_t src,
                                                                          std::uint8_t dst) {
    return TransportHeader(cat, typ, src, dst);
  }

 private:
  friend class AdcMessageBuilder;
  friend class MainBoardMessageBuilder;
  constexpr TransportHeader(MessageCategory cat, MessageType typ, std::uint8_t src,
                            std::uint8_t dst)
      : category(cat), type(typ), source_node_id(src), destination_node_id(dst) {}
};

}  // namespace midismith::protocol

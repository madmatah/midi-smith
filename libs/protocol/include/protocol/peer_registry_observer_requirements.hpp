#pragma once

#include <cstdint>

#include "protocol/peer_status.hpp"

namespace midismith::protocol {

class PeerRegistryObserverRequirements {
 public:
  virtual ~PeerRegistryObserverRequirements() = default;
  virtual void OnPeerChanged(std::uint8_t node_id, PeerStatus status) noexcept = 0;
};

}  // namespace midismith::protocol

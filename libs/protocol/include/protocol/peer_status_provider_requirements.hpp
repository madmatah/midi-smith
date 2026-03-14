#pragma once

#include <cstdint>

#include "protocol/peer_status.hpp"

namespace midismith::protocol {

class PeerStatusVisitorRequirements {
 public:
  virtual ~PeerStatusVisitorRequirements() = default;
  virtual void OnPeer(std::uint8_t node_id, PeerStatus status) noexcept = 0;
};

class PeerStatusProviderRequirements {
 public:
  virtual ~PeerStatusProviderRequirements() = default;
  virtual void ForEachActivePeer(PeerStatusVisitorRequirements& visitor) const noexcept = 0;
};

}  // namespace midismith::protocol

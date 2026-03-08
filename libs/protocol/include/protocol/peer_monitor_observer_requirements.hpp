#pragma once

#include "protocol/peer_status.hpp"

namespace midismith::protocol {

class PeerMonitorObserverRequirements {
 public:
  virtual ~PeerMonitorObserverRequirements() = default;
  virtual void OnPeerChanged(PeerStatus status) noexcept = 0;
};

}  // namespace midismith::protocol

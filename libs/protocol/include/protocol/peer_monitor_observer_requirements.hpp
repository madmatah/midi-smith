#pragma once

#include "protocol/messages.hpp"

namespace midismith::protocol {

class PeerMonitorObserverRequirements {
 public:
  virtual ~PeerMonitorObserverRequirements() = default;
  virtual void OnPeerHeartbeat(DeviceState device_state) noexcept = 0;
  virtual void OnPeerLost() noexcept = 0;
};

}  // namespace midismith::protocol

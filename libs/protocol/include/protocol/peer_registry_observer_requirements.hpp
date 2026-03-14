#pragma once

#include <cstdint>

#include "protocol/messages.hpp"

namespace midismith::protocol {

class PeerRegistryObserverRequirements {
 public:
  virtual ~PeerRegistryObserverRequirements() = default;
  virtual void OnPeerHeartbeat(std::uint8_t node_id, DeviceState device_state) noexcept = 0;
  virtual void OnPeerLost(std::uint8_t node_id) noexcept = 0;
};

}  // namespace midismith::protocol

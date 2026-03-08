#pragma once

#include <cstdint>

#include "protocol/messages.hpp"

namespace midismith::protocol {

enum class PeerConnectivity : std::uint8_t { kUnknown, kHealthy, kLost };

struct PeerStatus {
  PeerConnectivity connectivity;
  DeviceState device_state;
};

}  // namespace midismith::protocol

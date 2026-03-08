#pragma once

#include <cstdint>

#include "protocol/messages.hpp"
#include "protocol/peer_monitor_observer_requirements.hpp"
#include "protocol/peer_status.hpp"

namespace midismith::protocol {

class PeerMonitor {
 public:
  PeerMonitor(std::uint32_t timeout_ms, PeerMonitorObserverRequirements& observer) noexcept;

  void OnHeartbeatReceived(DeviceState state, std::uint32_t timestamp_ms) noexcept;
  void CheckTimeout(std::uint32_t current_time_ms) noexcept;

  [[nodiscard]] PeerStatus status() const noexcept;

 private:
  std::uint32_t timeout_ms_;
  PeerMonitorObserverRequirements& observer_;
  PeerConnectivity connectivity_;
  DeviceState last_device_state_;
  std::uint32_t last_heartbeat_timestamp_ms_;
};

}  // namespace midismith::protocol

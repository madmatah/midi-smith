#include "protocol/peer_monitor.hpp"

namespace midismith::protocol {

PeerMonitor::PeerMonitor(std::uint32_t timeout_ms,
                         PeerMonitorObserverRequirements& observer) noexcept
    : timeout_ms_(timeout_ms),
      observer_(observer),
      connectivity_(PeerConnectivity::kUnknown),
      last_device_state_(DeviceState::kInitializing),
      last_heartbeat_timestamp_ms_(0) {}

void PeerMonitor::OnHeartbeatReceived(DeviceState state, std::uint32_t timestamp_ms) noexcept {
  last_heartbeat_timestamp_ms_ = timestamp_ms;
  last_device_state_ = state;
  connectivity_ = PeerConnectivity::kHealthy;
  observer_.OnPeerHeartbeat(state);
}

void PeerMonitor::CheckTimeout(std::uint32_t current_time_ms) noexcept {
  if (connectivity_ != PeerConnectivity::kHealthy) {
    return;
  }
  if ((current_time_ms - last_heartbeat_timestamp_ms_) > timeout_ms_) {
    connectivity_ = PeerConnectivity::kLost;
    observer_.OnPeerLost();
  }
}

PeerStatus PeerMonitor::status() const noexcept {
  return {connectivity_, last_device_state_};
}

}  // namespace midismith::protocol

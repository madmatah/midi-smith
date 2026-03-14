#if defined(UNIT_TESTS)

#include "protocol/peer_monitor.hpp"

#include <catch2/catch_test_macros.hpp>

namespace {

using midismith::protocol::DeviceState;
using midismith::protocol::PeerConnectivity;
using midismith::protocol::PeerMonitor;
using midismith::protocol::PeerMonitorObserverRequirements;

class RecordingPeerMonitorObserver final : public PeerMonitorObserverRequirements {
 public:
  void OnPeerHeartbeat(DeviceState device_state) noexcept override {
    ++heartbeat_count_;
    last_device_state_ = device_state;
  }

  void OnPeerLost() noexcept override { ++lost_count_; }

  int heartbeat_count_{0};
  int lost_count_{0};
  DeviceState last_device_state_{DeviceState::kInitializing};
};

constexpr std::uint32_t kTimeoutMs = 1500;
constexpr std::uint32_t kTimestampMs = 1000;

}  // namespace

TEST_CASE("The PeerMonitor class", "[protocol][peer_monitor]") {
  RecordingPeerMonitorObserver observer;
  PeerMonitor monitor(kTimeoutMs, observer);

  SECTION("Initial state") {
    SECTION("Should be kUnknown with no notification") {
      REQUIRE(monitor.status().connectivity == PeerConnectivity::kUnknown);
      REQUIRE(observer.heartbeat_count_ == 0);
      REQUIRE(observer.lost_count_ == 0);
    }
  }

  SECTION("OnHeartbeatReceived()") {
    SECTION("First heartbeat should transition to kHealthy and notify") {
      monitor.OnHeartbeatReceived(DeviceState::kRunning, kTimestampMs);

      REQUIRE(monitor.status().connectivity == PeerConnectivity::kHealthy);
      REQUIRE(monitor.status().device_state == DeviceState::kRunning);
      REQUIRE(observer.heartbeat_count_ == 1);
      REQUIRE(observer.last_device_state_ == DeviceState::kRunning);
    }

    SECTION("Subsequent heartbeat with same DeviceState should still notify") {
      monitor.OnHeartbeatReceived(DeviceState::kRunning, kTimestampMs);
      const int count_after_first = observer.heartbeat_count_;

      monitor.OnHeartbeatReceived(DeviceState::kRunning, kTimestampMs + 500);

      REQUIRE(observer.heartbeat_count_ == count_after_first + 1);
    }

    SECTION("Subsequent heartbeat with different DeviceState should notify") {
      monitor.OnHeartbeatReceived(DeviceState::kRunning, kTimestampMs);
      const int count_after_first = observer.heartbeat_count_;

      monitor.OnHeartbeatReceived(DeviceState::kReady, kTimestampMs + 500);

      REQUIRE(observer.heartbeat_count_ == count_after_first + 1);
      REQUIRE(observer.last_device_state_ == DeviceState::kReady);
    }
  }

  SECTION("CheckTimeout()") {
    SECTION("Below timeout threshold should not notify") {
      monitor.OnHeartbeatReceived(DeviceState::kRunning, kTimestampMs);
      const int heartbeats_after_first = observer.heartbeat_count_;

      monitor.CheckTimeout(kTimestampMs + kTimeoutMs - 1);

      REQUIRE(observer.heartbeat_count_ == heartbeats_after_first);
      REQUIRE(observer.lost_count_ == 0);
      REQUIRE(monitor.status().connectivity == PeerConnectivity::kHealthy);
    }

    SECTION("At or above timeout threshold from kHealthy should transition to kLost and notify") {
      monitor.OnHeartbeatReceived(DeviceState::kRunning, kTimestampMs);

      monitor.CheckTimeout(kTimestampMs + kTimeoutMs + 1);

      REQUIRE(monitor.status().connectivity == PeerConnectivity::kLost);
      REQUIRE(observer.lost_count_ == 1);
    }

    SECTION("From kUnknown should not notify even if long time elapsed") {
      monitor.CheckTimeout(kTimestampMs + kTimeoutMs + 1);

      REQUIRE(observer.heartbeat_count_ == 0);
      REQUIRE(observer.lost_count_ == 0);
      REQUIRE(monitor.status().connectivity == PeerConnectivity::kUnknown);
    }
  }

  SECTION("Recovery after kLost") {
    SECTION("Heartbeat after kLost should transition to kHealthy and notify") {
      monitor.OnHeartbeatReceived(DeviceState::kRunning, kTimestampMs);
      monitor.CheckTimeout(kTimestampMs + kTimeoutMs + 1);
      const int heartbeats_after_lost = observer.heartbeat_count_;

      monitor.OnHeartbeatReceived(DeviceState::kRunning, kTimestampMs + kTimeoutMs + 100);

      REQUIRE(monitor.status().connectivity == PeerConnectivity::kHealthy);
      REQUIRE(observer.heartbeat_count_ == heartbeats_after_lost + 1);
    }
  }
}

#endif

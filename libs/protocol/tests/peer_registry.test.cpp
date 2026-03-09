#if defined(UNIT_TESTS)

#include "protocol/peer_registry.hpp"

#include <catch2/catch_test_macros.hpp>

namespace {

using midismith::protocol::DeviceState;
using midismith::protocol::PeerConnectivity;
using midismith::protocol::PeerRegistryObserverRequirements;
using midismith::protocol::PeerStatus;

class RecordingPeerRegistryObserver final : public PeerRegistryObserverRequirements {
 public:
  void OnPeerChanged(std::uint8_t node_id, PeerStatus status) noexcept override {
    ++notification_count_;
    last_node_id_ = node_id;
    last_status_ = status;
  }

  int notification_count_{0};
  std::uint8_t last_node_id_{0xFF};
  PeerStatus last_status_{PeerConnectivity::kUnknown, DeviceState::kReady};
};

constexpr std::uint32_t kTimeoutMs = 1500;
constexpr std::uint32_t kTimestampMs = 1000;

}  // namespace

TEST_CASE("The PeerRegistry class", "[protocol][peer_registry]") {
  RecordingPeerRegistryObserver observer;
  midismith::protocol::PeerRegistry<4> registry(kTimeoutMs, observer);

  SECTION("OnHeartbeatReceived()") {
    SECTION("New node_id should create a slot, transition to kHealthy, and notify with node_id") {
      registry.OnHeartbeatReceived(3, DeviceState::kRunning, kTimestampMs);

      REQUIRE(observer.notification_count_ == 1);
      REQUIRE(observer.last_node_id_ == 3);
      REQUIRE(observer.last_status_.connectivity == PeerConnectivity::kHealthy);
      REQUIRE(observer.last_status_.device_state == DeviceState::kRunning);
    }

    SECTION("Different node_ids should use separate slots") {
      registry.OnHeartbeatReceived(1, DeviceState::kRunning, kTimestampMs);
      registry.OnHeartbeatReceived(2, DeviceState::kReady, kTimestampMs);

      REQUIRE(observer.notification_count_ == 2);
      REQUIRE(observer.last_node_id_ == 2);
    }

    SECTION("Same node_id with same DeviceState should not notify after first heartbeat") {
      registry.OnHeartbeatReceived(1, DeviceState::kRunning, kTimestampMs);
      const int count_after_first = observer.notification_count_;

      registry.OnHeartbeatReceived(1, DeviceState::kRunning, kTimestampMs + 500);

      REQUIRE(observer.notification_count_ == count_after_first);
    }

    SECTION("Extra node_id beyond max capacity should be silently ignored") {
      registry.OnHeartbeatReceived(1, DeviceState::kRunning, kTimestampMs);
      registry.OnHeartbeatReceived(2, DeviceState::kRunning, kTimestampMs);
      registry.OnHeartbeatReceived(3, DeviceState::kRunning, kTimestampMs);
      registry.OnHeartbeatReceived(4, DeviceState::kRunning, kTimestampMs);
      const int count_at_capacity = observer.notification_count_;

      registry.OnHeartbeatReceived(5, DeviceState::kRunning, kTimestampMs);

      REQUIRE(observer.notification_count_ == count_at_capacity);
    }
  }

  SECTION("CheckTimeout()") {
    SECTION("Timeout should be tracked independently per slot") {
      registry.OnHeartbeatReceived(1, DeviceState::kRunning, kTimestampMs);
      registry.OnHeartbeatReceived(2, DeviceState::kRunning, kTimestampMs + 200);
      const int count_after_heartbeats = observer.notification_count_;

      registry.CheckTimeout(kTimestampMs + kTimeoutMs + 1);

      REQUIRE(observer.notification_count_ == count_after_heartbeats + 1);
      REQUIRE(observer.last_node_id_ == 1);
      REQUIRE(observer.last_status_.connectivity == PeerConnectivity::kLost);
    }

    SECTION("Should notify with correct node_id when peer times out") {
      registry.OnHeartbeatReceived(7, DeviceState::kRunning, kTimestampMs);

      registry.CheckTimeout(kTimestampMs + kTimeoutMs + 1);

      REQUIRE(observer.last_node_id_ == 7);
      REQUIRE(observer.last_status_.connectivity == PeerConnectivity::kLost);
    }

    SECTION("CheckTimeout should call all active monitors") {
      registry.OnHeartbeatReceived(1, DeviceState::kRunning, kTimestampMs);
      registry.OnHeartbeatReceived(2, DeviceState::kRunning, kTimestampMs);
      const int count_after_heartbeats = observer.notification_count_;

      registry.CheckTimeout(kTimestampMs + kTimeoutMs + 1);

      REQUIRE(observer.notification_count_ == count_after_heartbeats + 2);
    }
  }
}

#endif

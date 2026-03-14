#if defined(UNIT_TESTS)

#include "protocol/peer_registry.hpp"

#include <catch2/catch_test_macros.hpp>
#include <vector>

namespace {

using midismith::protocol::DeviceState;
using midismith::protocol::PeerConnectivity;
using midismith::protocol::PeerRegistryObserverRequirements;
using midismith::protocol::PeerStatus;
using midismith::protocol::PeerStatusVisitorRequirements;

class RecordingPeerRegistryObserver final : public PeerRegistryObserverRequirements {
 public:
  void OnPeerHeartbeat(std::uint8_t node_id, DeviceState device_state) noexcept override {
    ++heartbeat_count_;
    last_node_id_ = node_id;
    last_device_state_ = device_state;
  }

  void OnPeerLost(std::uint8_t node_id) noexcept override {
    ++lost_count_;
    last_lost_node_id_ = node_id;
  }

  int heartbeat_count_{0};
  int lost_count_{0};
  std::uint8_t last_node_id_{0xFF};
  std::uint8_t last_lost_node_id_{0xFF};
  DeviceState last_device_state_{DeviceState::kInitializing};
};

struct RecordedPeer {
  std::uint8_t node_id;
  PeerStatus status;
};

class RecordingPeerStatusVisitor final : public PeerStatusVisitorRequirements {
 public:
  void OnPeer(std::uint8_t node_id, PeerStatus status) noexcept override {
    peers_.push_back({node_id, status});
  }

  std::vector<RecordedPeer> peers_;
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

      REQUIRE(observer.heartbeat_count_ == 1);
      REQUIRE(observer.last_node_id_ == 3);
      REQUIRE(observer.last_device_state_ == DeviceState::kRunning);
    }

    SECTION("Different node_ids should use separate slots") {
      registry.OnHeartbeatReceived(1, DeviceState::kRunning, kTimestampMs);
      registry.OnHeartbeatReceived(2, DeviceState::kReady, kTimestampMs);

      REQUIRE(observer.heartbeat_count_ == 2);
      REQUIRE(observer.last_node_id_ == 2);
    }

    SECTION("Same node_id with same DeviceState should still notify on subsequent heartbeats") {
      registry.OnHeartbeatReceived(1, DeviceState::kRunning, kTimestampMs);
      const int count_after_first = observer.heartbeat_count_;

      registry.OnHeartbeatReceived(1, DeviceState::kRunning, kTimestampMs + 500);

      REQUIRE(observer.heartbeat_count_ == count_after_first + 1);
    }

    SECTION("Extra node_id beyond max capacity should be silently ignored") {
      registry.OnHeartbeatReceived(1, DeviceState::kRunning, kTimestampMs);
      registry.OnHeartbeatReceived(2, DeviceState::kRunning, kTimestampMs);
      registry.OnHeartbeatReceived(3, DeviceState::kRunning, kTimestampMs);
      registry.OnHeartbeatReceived(4, DeviceState::kRunning, kTimestampMs);
      const int count_at_capacity = observer.heartbeat_count_;

      registry.OnHeartbeatReceived(5, DeviceState::kRunning, kTimestampMs);

      REQUIRE(observer.heartbeat_count_ == count_at_capacity);
    }
  }

  SECTION("CheckTimeout()") {
    SECTION("Timeout should be tracked independently per slot") {
      registry.OnHeartbeatReceived(1, DeviceState::kRunning, kTimestampMs);
      registry.OnHeartbeatReceived(2, DeviceState::kRunning, kTimestampMs + 200);
      const int heartbeats_after_setup = observer.heartbeat_count_;

      registry.CheckTimeout(kTimestampMs + kTimeoutMs + 1);

      REQUIRE(observer.heartbeat_count_ == heartbeats_after_setup);
      REQUIRE(observer.lost_count_ == 1);
      REQUIRE(observer.last_lost_node_id_ == 1);
    }

    SECTION("Should notify with correct node_id when peer times out") {
      registry.OnHeartbeatReceived(7, DeviceState::kRunning, kTimestampMs);

      registry.CheckTimeout(kTimestampMs + kTimeoutMs + 1);

      REQUIRE(observer.lost_count_ == 1);
      REQUIRE(observer.last_lost_node_id_ == 7);
    }

    SECTION("CheckTimeout should call all active monitors") {
      registry.OnHeartbeatReceived(1, DeviceState::kRunning, kTimestampMs);
      registry.OnHeartbeatReceived(2, DeviceState::kRunning, kTimestampMs);

      registry.CheckTimeout(kTimestampMs + kTimeoutMs + 1);

      REQUIRE(observer.lost_count_ == 2);
    }
  }

  SECTION("ForEachActivePeer()") {
    SECTION("Should visit no peers when registry is empty") {
      RecordingPeerStatusVisitor visitor;

      registry.ForEachActivePeer(visitor);

      REQUIRE(visitor.peers_.empty());
    }

    SECTION("Should visit one healthy peer after receiving a heartbeat") {
      registry.OnHeartbeatReceived(3, DeviceState::kReady, kTimestampMs);
      RecordingPeerStatusVisitor visitor;

      registry.ForEachActivePeer(visitor);

      REQUIRE(visitor.peers_.size() == 1);
      REQUIRE(visitor.peers_[0].node_id == 3);
      REQUIRE(visitor.peers_[0].status.connectivity == PeerConnectivity::kHealthy);
      REQUIRE(visitor.peers_[0].status.device_state == DeviceState::kReady);
    }

    SECTION("Should visit a lost peer after timeout") {
      registry.OnHeartbeatReceived(5, DeviceState::kRunning, kTimestampMs);
      registry.CheckTimeout(kTimestampMs + kTimeoutMs + 1);
      RecordingPeerStatusVisitor visitor;

      registry.ForEachActivePeer(visitor);

      REQUIRE(visitor.peers_.size() == 1);
      REQUIRE(visitor.peers_[0].node_id == 5);
      REQUIRE(visitor.peers_[0].status.connectivity == PeerConnectivity::kLost);
    }

    SECTION("Should visit all active peers with their correct node_ids") {
      registry.OnHeartbeatReceived(1, DeviceState::kReady, kTimestampMs);
      registry.OnHeartbeatReceived(2, DeviceState::kRunning, kTimestampMs);
      registry.OnHeartbeatReceived(3, DeviceState::kInitializing, kTimestampMs);
      RecordingPeerStatusVisitor visitor;

      registry.ForEachActivePeer(visitor);

      REQUIRE(visitor.peers_.size() == 3);
    }
  }
}

#endif

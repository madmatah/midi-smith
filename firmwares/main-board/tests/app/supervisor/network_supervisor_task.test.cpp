#if defined(UNIT_TESTS)

#include "app/supervisor/network_supervisor_task.hpp"

#include <catch2/catch_test_macros.hpp>
#include <optional>
#include <queue>

#include "app/messaging/main_board_message_sender_requirements.hpp"
#include "os-types/queue_requirements.hpp"
#include "os-types/uptime_provider_requirements.hpp"
#include "protocol/messages.hpp"
#include "protocol/peer_registry_observer_requirements.hpp"

namespace {

class RecordingMessageSender final
    : public midismith::main_board::app::messaging::MainBoardMessageSenderRequirements {
 public:
  bool SendHeartbeat(midismith::protocol::DeviceState device_state) noexcept override {
    last_reported_state_ = device_state;
    heartbeat_count_++;
    return true;
  }

  bool SendStartAdc(std::uint8_t) noexcept override {
    return true;
  }
  bool SendStopAdc(std::uint8_t) noexcept override {
    return true;
  }
  bool SendStartCalibration(std::uint8_t, midismith::protocol::CalibMode) noexcept override {
    return true;
  }

  [[nodiscard]] std::optional<midismith::protocol::DeviceState> last_reported_state()
      const noexcept {
    return last_reported_state_;
  }
  [[nodiscard]] std::uint32_t heartbeat_count() const noexcept {
    return heartbeat_count_;
  }

 private:
  std::optional<midismith::protocol::DeviceState> last_reported_state_;
  std::uint32_t heartbeat_count_ = 0;
};

using Event = midismith::main_board::app::supervisor::NetworkSupervisorTask::Event;

class StubEventQueue final : public midismith::os::QueueRequirements<Event> {
 public:
  void Push(Event event) {
    pending_events_.push(event);
  }

  bool Send(const Event& item, std::uint32_t) noexcept override {
    pending_events_.push(item);
    return true;
  }

  bool SendFromIsr(const Event& item) noexcept override {
    pending_events_.push(item);
    return true;
  }

  bool Receive(Event& item, std::uint32_t) noexcept override {
    if (pending_events_.empty()) {
      return false;
    }
    item = pending_events_.front();
    pending_events_.pop();
    return true;
  }

 private:
  std::queue<Event> pending_events_;
};

class NullPeerRegistryObserver final
    : public midismith::protocol::PeerRegistryObserverRequirements {
 public:
  void OnPeerChanged(std::uint8_t, midismith::protocol::PeerStatus) noexcept override {}
};

class StubUptimeProvider final : public midismith::os::UptimeProviderRequirements {
 public:
  [[nodiscard]] std::uint32_t GetUptimeMs() const noexcept override {
    return uptime_ms_;
  }
  void set_uptime_ms(std::uint32_t ms) noexcept {
    uptime_ms_ = ms;
  }

 private:
  std::uint32_t uptime_ms_ = 0;
};

}  // namespace

using midismith::main_board::app::supervisor::NetworkSupervisorTask;
using midismith::protocol::DeviceState;

TEST_CASE("NetworkSupervisorTask — heartbeat emission") {
  RecordingMessageSender sender;
  StubEventQueue queue;
  NullPeerRegistryObserver peer_observer;
  StubUptimeProvider uptime;

  static constexpr std::uint32_t kPeerTimeoutMs = 1500;
  NetworkSupervisorTask task(sender, queue, peer_observer, uptime, kPeerTimeoutMs);

  SECTION("Should always send kRunning") {
    queue.Push(NetworkSupervisorTask::HeartbeatTick{});
    task.Run();

    REQUIRE(sender.last_reported_state() == DeviceState::kRunning);
  }

  SECTION("Should send one heartbeat per tick") {
    queue.Push(NetworkSupervisorTask::HeartbeatTick{});
    queue.Push(NetworkSupervisorTask::HeartbeatTick{});
    queue.Push(NetworkSupervisorTask::HeartbeatTick{});
    task.Run();

    REQUIRE(sender.heartbeat_count() == 3);
  }
}

#endif

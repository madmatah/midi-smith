#if defined(UNIT_TESTS)

#include "app/supervisor/adc_supervisor_task.hpp"

#include <catch2/catch_test_macros.hpp>
#include <optional>
#include <queue>

#include "app/analog/acquisition_state_requirements.hpp"
#include "app/messaging/adc_board_message_sender_requirements.hpp"
#include "os-types/queue_requirements.hpp"
#include "os-types/uptime_provider_requirements.hpp"
#include "protocol/messages.hpp"
#include "protocol/peer_monitor_observer_requirements.hpp"

namespace {

class RecordingMessageSender final
    : public midismith::adc_board::app::messaging::AdcBoardMessageSenderRequirements {
 public:
  bool SendNoteOn(std::uint8_t, std::uint8_t) noexcept override {
    return true;
  }
  bool SendNoteOff(std::uint8_t, std::uint8_t) noexcept override {
    return true;
  }

  bool SendHeartbeat(midismith::protocol::DeviceState device_state) noexcept override {
    last_reported_state_ = device_state;
    heartbeat_count_++;
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

class StubAcquisitionState final
    : public midismith::adc_board::app::analog::AcquisitionStateRequirements {
 public:
  explicit StubAcquisitionState(midismith::adc_board::app::analog::AcquisitionState state) noexcept
      : state_(state) {}

  midismith::adc_board::app::analog::AcquisitionState GetState() const noexcept override {
    return state_;
  }

  void set_state(midismith::adc_board::app::analog::AcquisitionState state) noexcept {
    state_ = state;
  }

 private:
  midismith::adc_board::app::analog::AcquisitionState state_;
};

using Event = midismith::adc_board::app::supervisor::AdcSupervisorTask::Event;

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

class NullPeerMonitorObserver final : public midismith::protocol::PeerMonitorObserverRequirements {
 public:
  void OnPeerChanged(midismith::protocol::PeerStatus) noexcept override {}
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

using midismith::adc_board::app::analog::AcquisitionState;
using midismith::adc_board::app::supervisor::AdcSupervisorTask;
using midismith::protocol::DeviceState;

TEST_CASE("AdcSupervisorTask — heartbeat state mapping") {
  RecordingMessageSender sender;
  StubEventQueue queue;
  NullPeerMonitorObserver peer_observer;
  StubUptimeProvider uptime;

  static constexpr std::uint32_t kPeerTimeoutMs = 1500;

  SECTION("Should send kIdle when acquisition is disabled") {
    StubAcquisitionState acquisition_state(AcquisitionState::kDisabled);
    AdcSupervisorTask task(sender, acquisition_state, queue, peer_observer, uptime, kPeerTimeoutMs);

    queue.Push(AdcSupervisorTask::HeartbeatTick{});
    task.Run();

    REQUIRE(sender.last_reported_state() == DeviceState::kIdle);
  }

  SECTION("Should send kRunning when acquisition is enabled") {
    StubAcquisitionState acquisition_state(AcquisitionState::kEnabled);
    AdcSupervisorTask task(sender, acquisition_state, queue, peer_observer, uptime, kPeerTimeoutMs);

    queue.Push(AdcSupervisorTask::HeartbeatTick{});
    task.Run();

    REQUIRE(sender.last_reported_state() == DeviceState::kRunning);
  }

  SECTION("Should send one heartbeat per tick") {
    StubAcquisitionState acquisition_state(AcquisitionState::kDisabled);
    AdcSupervisorTask task(sender, acquisition_state, queue, peer_observer, uptime, kPeerTimeoutMs);

    queue.Push(AdcSupervisorTask::HeartbeatTick{});
    queue.Push(AdcSupervisorTask::HeartbeatTick{});
    queue.Push(AdcSupervisorTask::HeartbeatTick{});
    task.Run();

    REQUIRE(sender.heartbeat_count() == 3);
  }

  SECTION("Should reflect state changes between ticks") {
    StubAcquisitionState acquisition_state(AcquisitionState::kDisabled);
    AdcSupervisorTask task(sender, acquisition_state, queue, peer_observer, uptime, kPeerTimeoutMs);

    queue.Push(AdcSupervisorTask::HeartbeatTick{});
    task.Run();
    REQUIRE(sender.last_reported_state() == DeviceState::kIdle);

    acquisition_state.set_state(AcquisitionState::kEnabled);
    queue.Push(AdcSupervisorTask::HeartbeatTick{});
    task.Run();
    REQUIRE(sender.last_reported_state() == DeviceState::kRunning);
  }
}

#endif

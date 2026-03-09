#if defined(UNIT_TESTS)

#include "app/supervisor/network_supervisor_task.hpp"

#include <catch2/catch_test_macros.hpp>
#include <optional>
#include <queue>

#include "app/messaging/main_board_message_sender_requirements.hpp"
#include "domain/adc/adc_board_power_switch_requirements.hpp"
#include "domain/adc/adc_board_state.hpp"
#include "domain/adc/adc_boards_controller.hpp"
#include "os-types/queue_requirements.hpp"
#include "os-types/uptime_provider_requirements.hpp"
#include "protocol/messages.hpp"

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

class NullAdcBoardPowerSwitch final
    : public midismith::main_board::domain::adc::AdcBoardPowerSwitchRequirements {
 public:
  void PowerOn(std::uint8_t) noexcept override {}
  void PowerOff(std::uint8_t) noexcept override {}
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
using midismith::main_board::domain::adc::AdcBoardsController;
using midismith::main_board::domain::adc::AdcBoardState;
using midismith::protocol::DeviceState;

TEST_CASE("NetworkSupervisorTask — heartbeat emission") {
  RecordingMessageSender sender;
  StubEventQueue queue;
  NullAdcBoardPowerSwitch power_switch;
  StubUptimeProvider uptime;
  AdcBoardsController<8> boards_controller(power_switch, 5000, uptime);

  static constexpr std::uint32_t kPeerTimeoutMs = 1500;
  NetworkSupervisorTask task(sender, queue, boards_controller, uptime, kPeerTimeoutMs);

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

TEST_CASE("NetworkSupervisorTask — PowerOnCommand dispatch") {
  RecordingMessageSender sender;
  StubEventQueue queue;
  NullAdcBoardPowerSwitch power_switch;
  StubUptimeProvider uptime;
  AdcBoardsController<8> boards_controller(power_switch, 5000, uptime);

  NetworkSupervisorTask task(sender, queue, boards_controller, uptime, 1500);

  SECTION("PowerOnCommand should set the board to kElectricallyOn") {
    queue.Push(NetworkSupervisorTask::PowerOnCommand{.peer_id = 1});
    task.Run();

    REQUIRE(boards_controller.board_state(1) == AdcBoardState::kElectricallyOn);
  }
}

TEST_CASE("NetworkSupervisorTask — PowerOffCommand dispatch") {
  RecordingMessageSender sender;
  StubEventQueue queue;
  NullAdcBoardPowerSwitch power_switch;
  StubUptimeProvider uptime;
  AdcBoardsController<8> boards_controller(power_switch, 5000, uptime);

  NetworkSupervisorTask task(sender, queue, boards_controller, uptime, 1500);

  SECTION("PowerOffCommand after PowerOnCommand should set the board back to kElectricallyOff") {
    queue.Push(NetworkSupervisorTask::PowerOnCommand{.peer_id = 1});
    queue.Push(NetworkSupervisorTask::PowerOffCommand{.peer_id = 1});
    task.Run();

    REQUIRE(boards_controller.board_state(1) == AdcBoardState::kElectricallyOff);
  }
}

TEST_CASE("NetworkSupervisorTask — StartPowerSequenceCommand dispatch") {
  RecordingMessageSender sender;
  StubEventQueue queue;
  NullAdcBoardPowerSwitch power_switch;
  StubUptimeProvider uptime;
  AdcBoardsController<8> boards_controller(power_switch, 5000, uptime);

  NetworkSupervisorTask task(sender, queue, boards_controller, uptime, 1500);

  SECTION("StartPowerSequenceCommand should start the power-on sequence") {
    queue.Push(NetworkSupervisorTask::StartPowerSequenceCommand{});
    task.Run();

    REQUIRE(boards_controller.board_state(1) == AdcBoardState::kElectricallyOn);
  }
}

#endif

#if defined(UNIT_TESTS)

#include "app/supervisor/adc_supervisor_task.hpp"

#include <catch2/catch_test_macros.hpp>

#include "adc_supervisor_test_support.hpp"
#include "protocol/messages.hpp"

using midismith::adc_board::app::analog::AcquisitionState;
using midismith::adc_board::app::supervisor::AdcSupervisorTask;
using midismith::adc_board::app::supervisor::test::NullPeerMonitorObserver;
using midismith::adc_board::app::supervisor::test::RecordingMessageSender;
using midismith::adc_board::app::supervisor::test::RecordingPeerMonitorObserver;
using midismith::adc_board::app::supervisor::test::StubAcquisitionState;
using midismith::adc_board::app::supervisor::test::StubEventQueue;
using midismith::adc_board::app::supervisor::test::StubUptimeProvider;
using midismith::protocol::DeviceState;
using midismith::protocol::PeerConnectivity;

TEST_CASE("The AdcSupervisorTask class") {
  SECTION("The Run() method") {
    SECTION("When the task starts with acquisition disabled") {
      SECTION("Should emit one startup heartbeat with kIdle state") {
        RecordingMessageSender sender;
        StubEventQueue queue;
        NullPeerMonitorObserver peer_observer;
        StubUptimeProvider uptime;
        StubAcquisitionState acquisition_state(AcquisitionState::kDisabled);
        static constexpr std::uint32_t kPeerTimeoutMs = 1500;
        AdcSupervisorTask task(sender, acquisition_state, queue, peer_observer, uptime,
                               kPeerTimeoutMs);

        task.Run();

        REQUIRE(sender.heartbeat_count() == 1);
        REQUIRE(sender.last_reported_state() == DeviceState::kIdle);
      }
    }

    SECTION("When the task starts with acquisition enabled") {
      SECTION("Should emit one startup heartbeat with kRunning state") {
        RecordingMessageSender sender;
        StubEventQueue queue;
        NullPeerMonitorObserver peer_observer;
        StubUptimeProvider uptime;
        StubAcquisitionState acquisition_state(AcquisitionState::kEnabled);
        static constexpr std::uint32_t kPeerTimeoutMs = 1500;
        AdcSupervisorTask task(sender, acquisition_state, queue, peer_observer, uptime,
                               kPeerTimeoutMs);

        task.Run();

        REQUIRE(sender.heartbeat_count() == 1);
        REQUIRE(sender.last_reported_state() == DeviceState::kRunning);
      }
    }

    SECTION("When a HeartbeatTick event is received") {
      SECTION("Should publish heartbeat according to current acquisition state") {
        RecordingMessageSender sender;
        StubEventQueue queue;
        NullPeerMonitorObserver peer_observer;
        StubUptimeProvider uptime;
        StubAcquisitionState acquisition_state(AcquisitionState::kDisabled);
        static constexpr std::uint32_t kPeerTimeoutMs = 1500;
        AdcSupervisorTask task(sender, acquisition_state, queue, peer_observer, uptime,
                               kPeerTimeoutMs);

        queue.Push(AdcSupervisorTask::HeartbeatTick{});
        task.Run();
        REQUIRE(sender.last_reported_state() == DeviceState::kIdle);

        acquisition_state.set_state(AcquisitionState::kEnabled);
        queue.Push(AdcSupervisorTask::HeartbeatTick{});
        task.Run();
        REQUIRE(sender.last_reported_state() == DeviceState::kRunning);
      }
    }

    SECTION("When multiple HeartbeatTick events are received") {
      SECTION("Should emit one heartbeat per tick in addition to startup heartbeat") {
        RecordingMessageSender sender;
        StubEventQueue queue;
        NullPeerMonitorObserver peer_observer;
        StubUptimeProvider uptime;
        StubAcquisitionState acquisition_state(AcquisitionState::kDisabled);
        static constexpr std::uint32_t kPeerTimeoutMs = 1500;
        AdcSupervisorTask task(sender, acquisition_state, queue, peer_observer, uptime,
                               kPeerTimeoutMs);

        queue.Push(AdcSupervisorTask::HeartbeatTick{});
        queue.Push(AdcSupervisorTask::HeartbeatTick{});
        queue.Push(AdcSupervisorTask::HeartbeatTick{});
        task.Run();

        REQUIRE(sender.heartbeat_count() == 4);
      }
    }

    SECTION("When a HeartbeatReceived event is received") {
      SECTION("Should notify peer observer with kHealthy connectivity") {
        RecordingMessageSender sender;
        StubEventQueue queue;
        RecordingPeerMonitorObserver peer_observer;
        StubUptimeProvider uptime;
        StubAcquisitionState acquisition_state(AcquisitionState::kDisabled);
        static constexpr std::uint32_t kPeerTimeoutMs = 1500;
        AdcSupervisorTask task(sender, acquisition_state, queue, peer_observer, uptime,
                               kPeerTimeoutMs);

        uptime.set_uptime_ms(100);
        queue.Push(AdcSupervisorTask::HeartbeatReceived{.device_state = DeviceState::kRunning});
        task.Run();

        REQUIRE(peer_observer.statuses().size() == 1);
        REQUIRE(peer_observer.statuses().front().connectivity == PeerConnectivity::kHealthy);
        REQUIRE(peer_observer.statuses().front().device_state == DeviceState::kRunning);
      }
    }

    SECTION("When timeout is checked after a previously received heartbeat") {
      SECTION("Should notify peer observer with kLost connectivity") {
        RecordingMessageSender sender;
        StubEventQueue queue;
        RecordingPeerMonitorObserver peer_observer;
        StubUptimeProvider uptime;
        StubAcquisitionState acquisition_state(AcquisitionState::kDisabled);
        static constexpr std::uint32_t kPeerTimeoutMs = 1500;
        AdcSupervisorTask task(sender, acquisition_state, queue, peer_observer, uptime,
                               kPeerTimeoutMs);

        uptime.set_uptime_ms(100);
        queue.Push(AdcSupervisorTask::HeartbeatReceived{.device_state = DeviceState::kIdle});
        task.Run();
        REQUIRE(peer_observer.statuses().size() == 1);
        REQUIRE(peer_observer.statuses().front().connectivity == PeerConnectivity::kHealthy);

        uptime.set_uptime_ms(1701);
        queue.Push(AdcSupervisorTask::TimeoutCheckTick{});
        task.Run();

        REQUIRE(peer_observer.statuses().size() == 2);
        REQUIRE(peer_observer.statuses().back().connectivity == PeerConnectivity::kLost);
        REQUIRE(peer_observer.statuses().back().device_state == DeviceState::kIdle);
      }
    }
  }
}

#endif

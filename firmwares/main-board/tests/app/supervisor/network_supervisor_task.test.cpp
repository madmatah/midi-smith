#if defined(UNIT_TESTS)

#include "app/supervisor/network_supervisor_task.hpp"

#include <catch2/catch_test_macros.hpp>

#include "app/adc/adc_board_state.hpp"
#include "app/adc/adc_boards_controller.hpp"
#include "network_supervisor_test_support.hpp"
#include "protocol/messages.hpp"

using midismith::main_board::app::adc::AdcBoardsController;
using midismith::main_board::app::adc::AdcBoardState;
using midismith::main_board::app::supervisor::NetworkSupervisorTask;
using midismith::main_board::app::supervisor::test::NullAdcBoardPowerSwitch;
using midismith::main_board::app::supervisor::test::RecordingMessageSender;
using midismith::main_board::app::supervisor::test::StubEventQueue;
using midismith::main_board::app::supervisor::test::StubUptimeProvider;
using midismith::protocol::DeviceState;

TEST_CASE("The NetworkSupervisorTask class") {
  SECTION("The Run() method") {
    SECTION("When a HeartbeatTick event is received") {
      SECTION("Should send heartbeat with kRunning state") {
        RecordingMessageSender sender;
        StubEventQueue queue;
        NullAdcBoardPowerSwitch power_switch;
        StubUptimeProvider uptime;
        AdcBoardsController<8> boards_controller(sender, power_switch, 5000, uptime);
        static constexpr std::uint32_t kPeerTimeoutMs = 1500;
        NetworkSupervisorTask task(sender, queue, boards_controller, uptime, kPeerTimeoutMs);

        queue.Push(NetworkSupervisorTask::HeartbeatTick{});
        task.Run();

        REQUIRE(sender.last_reported_state() == DeviceState::kRunning);
      }
    }

    SECTION("When multiple HeartbeatTick events are received") {
      SECTION("Should send one heartbeat per tick") {
        RecordingMessageSender sender;
        StubEventQueue queue;
        NullAdcBoardPowerSwitch power_switch;
        StubUptimeProvider uptime;
        AdcBoardsController<8> boards_controller(sender, power_switch, 5000, uptime);
        static constexpr std::uint32_t kPeerTimeoutMs = 1500;
        NetworkSupervisorTask task(sender, queue, boards_controller, uptime, kPeerTimeoutMs);

        queue.Push(NetworkSupervisorTask::HeartbeatTick{});
        queue.Push(NetworkSupervisorTask::HeartbeatTick{});
        queue.Push(NetworkSupervisorTask::HeartbeatTick{});
        task.Run();

        REQUIRE(sender.heartbeat_count() == 3);
      }
    }

    SECTION("When a PowerOnCommand event is received") {
      SECTION("Should set targeted board to kElectricallyOn") {
        RecordingMessageSender sender;
        StubEventQueue queue;
        NullAdcBoardPowerSwitch power_switch;
        StubUptimeProvider uptime;
        AdcBoardsController<8> boards_controller(sender, power_switch, 5000, uptime);
        NetworkSupervisorTask task(sender, queue, boards_controller, uptime, 1500);

        queue.Push(NetworkSupervisorTask::PowerOnCommand{.peer_id = 1});
        task.Run();

        REQUIRE(boards_controller.board_state(1) == AdcBoardState::kElectricallyOn);
      }
    }

    SECTION("When a PowerOffCommand event is received after power on") {
      SECTION("Should set targeted board to kElectricallyOff") {
        RecordingMessageSender sender;
        StubEventQueue queue;
        NullAdcBoardPowerSwitch power_switch;
        StubUptimeProvider uptime;
        AdcBoardsController<8> boards_controller(sender, power_switch, 5000, uptime);
        NetworkSupervisorTask task(sender, queue, boards_controller, uptime, 1500);

        queue.Push(NetworkSupervisorTask::PowerOnCommand{.peer_id = 1});
        queue.Push(NetworkSupervisorTask::PowerOffCommand{.peer_id = 1});
        task.Run();

        REQUIRE(boards_controller.board_state(1) == AdcBoardState::kElectricallyOff);
      }
    }

    SECTION("When a StartPowerSequenceCommand event is received") {
      SECTION("Should start startup sequence by powering on board 1") {
        RecordingMessageSender sender;
        StubEventQueue queue;
        NullAdcBoardPowerSwitch power_switch;
        StubUptimeProvider uptime;
        AdcBoardsController<8> boards_controller(sender, power_switch, 5000, uptime);
        NetworkSupervisorTask task(sender, queue, boards_controller, uptime, 1500);

        queue.Push(NetworkSupervisorTask::StartPowerSequenceCommand{});
        task.Run();

        REQUIRE(boards_controller.board_state(1) == AdcBoardState::kElectricallyOn);
      }
    }

    SECTION("When timeout check occurs during startup sequence with a silent board") {
      SECTION("Should mark current board unresponsive and continue with next board") {
        RecordingMessageSender sender;
        StubEventQueue queue;
        NullAdcBoardPowerSwitch power_switch;
        StubUptimeProvider uptime;
        AdcBoardsController<8> boards_controller(sender, power_switch, 1500, uptime);
        NetworkSupervisorTask task(sender, queue, boards_controller, uptime, 1500);

        uptime.set_uptime_ms(0);
        queue.Push(NetworkSupervisorTask::StartPowerSequenceCommand{});
        task.Run();
        REQUIRE(boards_controller.board_state(1) == AdcBoardState::kElectricallyOn);

        uptime.set_uptime_ms(1501);
        queue.Push(NetworkSupervisorTask::TimeoutCheckTick{});
        task.Run();

        REQUIRE(boards_controller.board_state(1) == AdcBoardState::kUnresponsive);
        REQUIRE(boards_controller.board_state(2) == AdcBoardState::kElectricallyOn);
      }
    }

    SECTION("When heartbeat is received then peer times out then heartbeat is received again") {
      SECTION("Should transition board state reachable then electrically on then reachable") {
        RecordingMessageSender sender;
        StubEventQueue queue;
        NullAdcBoardPowerSwitch power_switch;
        StubUptimeProvider uptime;
        AdcBoardsController<8> boards_controller(sender, power_switch, 5000, uptime);
        NetworkSupervisorTask task(sender, queue, boards_controller, uptime, 1000);

        queue.Push(NetworkSupervisorTask::PowerOnCommand{.peer_id = 1});
        task.Run();
        REQUIRE(boards_controller.board_state(1) == AdcBoardState::kElectricallyOn);

        uptime.set_uptime_ms(100);
        queue.Push(NetworkSupervisorTask::HeartbeatReceived{.node_id = 1,
                                                            .device_state = DeviceState::kRunning});
        task.Run();
        REQUIRE(boards_controller.board_state(1) == AdcBoardState::kReachable);

        uptime.set_uptime_ms(1201);
        queue.Push(NetworkSupervisorTask::TimeoutCheckTick{});
        task.Run();
        REQUIRE(boards_controller.board_state(1) == AdcBoardState::kElectricallyOn);

        uptime.set_uptime_ms(1300);
        queue.Push(NetworkSupervisorTask::HeartbeatReceived{.node_id = 1,
                                                            .device_state = DeviceState::kRunning});
        task.Run();
        REQUIRE(boards_controller.board_state(1) == AdcBoardState::kReachable);
      }
    }

    SECTION("When a StopAllCommand event is received") {
      SECTION("Should power off all previously powered boards") {
        RecordingMessageSender sender;
        StubEventQueue queue;
        NullAdcBoardPowerSwitch power_switch;
        StubUptimeProvider uptime;
        AdcBoardsController<8> boards_controller(sender, power_switch, 5000, uptime);
        NetworkSupervisorTask task(sender, queue, boards_controller, uptime, 1500);

        queue.Push(NetworkSupervisorTask::PowerOnCommand{.peer_id = 1});
        queue.Push(NetworkSupervisorTask::PowerOnCommand{.peer_id = 2});
        queue.Push(NetworkSupervisorTask::StopAllCommand{});
        task.Run();

        REQUIRE(boards_controller.board_state(1) == AdcBoardState::kElectricallyOff);
        REQUIRE(boards_controller.board_state(2) == AdcBoardState::kElectricallyOff);
      }
    }
  }
}

#endif

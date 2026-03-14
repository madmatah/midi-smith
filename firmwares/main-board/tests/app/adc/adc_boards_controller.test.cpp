#if defined(UNIT_TESTS)

#include "app/adc/adc_boards_controller.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <vector>

#include "app/adc/adc_board_power_switch_requirements.hpp"
#include "app/adc/adc_board_state.hpp"
#include "app/messaging/main_board_message_sender_requirements.hpp"
#include "os-types/uptime_provider_requirements.hpp"
#include "protocol/messages.hpp"

namespace {

class RecordingMessageSender final
    : public midismith::main_board::app::messaging::MainBoardMessageSenderRequirements {
 public:
  bool SendHeartbeat(midismith::protocol::DeviceState) noexcept override {
    ++heartbeat_count_;
    return true;
  }

  bool SendStartAdc(std::uint8_t target_node_id) noexcept override {
    start_adc_calls_.push_back(target_node_id);
    return true;
  }

  bool SendStopAdc(std::uint8_t) noexcept override {
    return true;
  }

  bool SendStartCalibration(std::uint8_t, midismith::protocol::CalibMode) noexcept override {
    return true;
  }

  [[nodiscard]] int heartbeat_count() const noexcept {
    return heartbeat_count_;
  }

  void reset_heartbeat_count() noexcept {
    heartbeat_count_ = 0;
  }

  [[nodiscard]] const std::vector<std::uint8_t>& start_adc_calls() const noexcept {
    return start_adc_calls_;
  }

 private:
  int heartbeat_count_ = 0;
  std::vector<std::uint8_t> start_adc_calls_;
};

class RecordingPowerSwitch final
    : public midismith::main_board::app::adc::AdcBoardPowerSwitchRequirements {
 public:
  void PowerOn(std::uint8_t peer_id) noexcept override {
    power_on_calls_.push_back(peer_id);
  }

  void PowerOff(std::uint8_t peer_id) noexcept override {
    power_off_calls_.push_back(peer_id);
  }

  [[nodiscard]] const std::vector<std::uint8_t>& power_on_calls() const noexcept {
    return power_on_calls_;
  }

  [[nodiscard]] const std::vector<std::uint8_t>& power_off_calls() const noexcept {
    return power_off_calls_;
  }

  [[nodiscard]] std::size_t power_on_count() const noexcept {
    return power_on_calls_.size();
  }

 private:
  std::vector<std::uint8_t> power_on_calls_;
  std::vector<std::uint8_t> power_off_calls_;
};

class StubUptimeProvider final : public midismith::os::UptimeProviderRequirements {
 public:
  [[nodiscard]] std::uint32_t GetUptimeMs() const noexcept override {
    return uptime_ms_;
  }

  void set_uptime_ms(std::uint32_t uptime_ms) noexcept {
    uptime_ms_ = uptime_ms;
  }

 private:
  std::uint32_t uptime_ms_ = 0;
};

constexpr std::uint32_t kPowerOnTimeoutMs = 1000;
constexpr std::size_t kBoardCount = 3;
constexpr std::size_t kMaxBoardCount = 8;

using Controller = midismith::main_board::app::adc::AdcBoardsController<kBoardCount>;
using ControllerMax = midismith::main_board::app::adc::AdcBoardsController<kMaxBoardCount>;
using AdcBoardState = midismith::main_board::app::adc::AdcBoardState;
using DeviceState = midismith::protocol::DeviceState;

}  // namespace

TEST_CASE("The AdcBoardsController class") {
  SECTION("The board_state() method") {
    SECTION("When the controller is newly created") {
      SECTION("Should report all boards as kElectricallyOff") {
        RecordingMessageSender sender;
        RecordingPowerSwitch power_switch;
        StubUptimeProvider uptime;
        Controller controller(sender, power_switch, kPowerOnTimeoutMs, uptime);

        for (std::uint8_t id = 1; id <= kBoardCount; ++id) {
          REQUIRE(controller.board_state(id) == AdcBoardState::kElectricallyOff);
        }
      }
    }

    SECTION("When an invalid board id is requested") {
      SECTION("Should return kElectricallyOff") {
        RecordingMessageSender sender;
        RecordingPowerSwitch power_switch;
        StubUptimeProvider uptime;
        Controller controller(sender, power_switch, kPowerOnTimeoutMs, uptime);

        REQUIRE(controller.board_state(0) == AdcBoardState::kElectricallyOff);
        REQUIRE(controller.board_state(kBoardCount + 1) == AdcBoardState::kElectricallyOff);
      }
    }
  }

  SECTION("The PowerOn() method") {
    SECTION("When a valid board id is provided") {
      SECTION("Should set board state to kElectricallyOn and call power switch") {
        RecordingMessageSender sender;
        RecordingPowerSwitch power_switch;
        StubUptimeProvider uptime;
        Controller controller(sender, power_switch, kPowerOnTimeoutMs, uptime);

        controller.PowerOn(1);

        REQUIRE(controller.board_state(1) == AdcBoardState::kElectricallyOn);
        REQUIRE(power_switch.power_on_calls().size() == 1);
        REQUIRE(power_switch.power_on_calls().at(0) == 1);
      }
    }

    SECTION("When an invalid board id is provided") {
      SECTION("Should be a no-op") {
        RecordingMessageSender sender;
        RecordingPowerSwitch power_switch;
        StubUptimeProvider uptime;
        Controller controller(sender, power_switch, kPowerOnTimeoutMs, uptime);

        controller.PowerOn(0);
        controller.PowerOn(kBoardCount + 1);

        REQUIRE(power_switch.power_on_calls().empty());
      }
    }
  }

  SECTION("The PowerOff() method") {
    SECTION("When a valid board id is provided after power on") {
      SECTION("Should set board state back to kElectricallyOff") {
        RecordingMessageSender sender;
        RecordingPowerSwitch power_switch;
        StubUptimeProvider uptime;
        Controller controller(sender, power_switch, kPowerOnTimeoutMs, uptime);

        controller.PowerOn(1);
        controller.PowerOff(1);

        REQUIRE(controller.board_state(1) == AdcBoardState::kElectricallyOff);
        REQUIRE(power_switch.power_off_calls().size() == 1);
        REQUIRE(power_switch.power_off_calls().at(0) == 1);
      }
    }

    SECTION("When an invalid board id is provided") {
      SECTION("Should be a no-op") {
        RecordingMessageSender sender;
        RecordingPowerSwitch power_switch;
        StubUptimeProvider uptime;
        Controller controller(sender, power_switch, kPowerOnTimeoutMs, uptime);

        controller.PowerOff(0);
        controller.PowerOff(kBoardCount + 1);

        REQUIRE(power_switch.power_off_calls().empty());
      }
    }
  }

  SECTION("The OnPeerHeartbeat() method") {
    SECTION("When a healthy peer reports kInitializing") {
      SECTION("Should mark the board kInitializing and emit a heartbeat") {
        RecordingMessageSender sender;
        RecordingPowerSwitch power_switch;
        StubUptimeProvider uptime;
        Controller controller(sender, power_switch, kPowerOnTimeoutMs, uptime);

        controller.PowerOn(1);
        controller.OnPeerHeartbeat(1, DeviceState::kInitializing);

        REQUIRE(controller.board_state(1) == AdcBoardState::kInitializing);
        REQUIRE(sender.heartbeat_count() == 1);
      }
    }

    SECTION("When a healthy peer reports kReady") {
      SECTION("Should mark the board kReady and send StartAdc") {
        RecordingMessageSender sender;
        RecordingPowerSwitch power_switch;
        StubUptimeProvider uptime;
        Controller controller(sender, power_switch, kPowerOnTimeoutMs, uptime);

        controller.PowerOn(1);
        controller.OnPeerHeartbeat(1, DeviceState::kReady);

        REQUIRE(controller.board_state(1) == AdcBoardState::kReady);
        REQUIRE(sender.start_adc_calls().size() == 1);
        REQUIRE(sender.start_adc_calls().at(0) == 1);
      }
    }

    SECTION("When a healthy peer reports kRunning") {
      SECTION("Should mark the board kAcquiring") {
        RecordingMessageSender sender;
        RecordingPowerSwitch power_switch;
        StubUptimeProvider uptime;
        Controller controller(sender, power_switch, kPowerOnTimeoutMs, uptime);

        controller.PowerOn(1);
        controller.OnPeerHeartbeat(1, DeviceState::kRunning);

        REQUIRE(controller.board_state(1) == AdcBoardState::kAcquiring);
      }
    }

    SECTION("When an invalid board id is provided") {
      SECTION("Should not affect current startup sequence state") {
        RecordingMessageSender sender;
        RecordingPowerSwitch power_switch;
        StubUptimeProvider uptime;
        ControllerMax controller(sender, power_switch, kPowerOnTimeoutMs, uptime);

        controller.StartPowerSequence();
        REQUIRE(controller.board_state(1) == AdcBoardState::kElectricallyOn);
        REQUIRE(controller.board_state(2) == AdcBoardState::kElectricallyOff);

        controller.OnPeerHeartbeat(0, DeviceState::kInitializing);
        controller.OnPeerHeartbeat(kMaxBoardCount + 1, DeviceState::kInitializing);

        REQUIRE(controller.board_state(1) == AdcBoardState::kElectricallyOn);
        REQUIRE(controller.board_state(2) == AdcBoardState::kElectricallyOff);
      }
    }
  }

  SECTION("The OnPeerLost() method") {
    SECTION("When a lost status is received for a connected board") {
      SECTION("Should mark the board electrically on") {
        RecordingMessageSender sender;
        RecordingPowerSwitch power_switch;
        StubUptimeProvider uptime;
        Controller controller(sender, power_switch, kPowerOnTimeoutMs, uptime);

        controller.PowerOn(1);
        controller.OnPeerHeartbeat(1, DeviceState::kInitializing);
        controller.OnPeerLost(1);

        REQUIRE(controller.board_state(1) == AdcBoardState::kElectricallyOn);
      }
    }

    SECTION("When a board reconnects after being lost") {
      SECTION("Should emit a new heartbeat") {
        RecordingMessageSender sender;
        RecordingPowerSwitch power_switch;
        StubUptimeProvider uptime;
        Controller controller(sender, power_switch, kPowerOnTimeoutMs, uptime);

        controller.PowerOn(1);
        controller.OnPeerHeartbeat(1, DeviceState::kInitializing);
        controller.OnPeerLost(1);
        sender.reset_heartbeat_count();

        controller.OnPeerHeartbeat(1, DeviceState::kInitializing);

        REQUIRE(sender.heartbeat_count() == 1);
      }
    }
  }

  SECTION("The StartPowerSequence() method") {
    SECTION("When called for the first time") {
      SECTION("Should power on board 1") {
        RecordingMessageSender sender;
        RecordingPowerSwitch power_switch;
        StubUptimeProvider uptime;
        Controller controller(sender, power_switch, kPowerOnTimeoutMs, uptime);

        controller.StartPowerSequence();

        REQUIRE(controller.board_state(1) == AdcBoardState::kElectricallyOn);
        REQUIRE(power_switch.power_on_calls().size() == 1);
        REQUIRE(power_switch.power_on_calls().at(0) == 1);
      }
    }

    SECTION("When called while a sequence is already running") {
      SECTION("Should ignore the second call") {
        RecordingMessageSender sender;
        RecordingPowerSwitch power_switch;
        StubUptimeProvider uptime;
        Controller controller(sender, power_switch, kPowerOnTimeoutMs, uptime);

        controller.StartPowerSequence();
        controller.StartPowerSequence();

        REQUIRE(power_switch.power_on_count() == 1);
      }
    }

    SECTION("When a peer is already healthy before sequence start") {
      SECTION("Should advance sequence on the next heartbeat from that peer") {
        RecordingMessageSender sender;
        RecordingPowerSwitch power_switch;
        StubUptimeProvider uptime;
        ControllerMax controller(sender, power_switch, kPowerOnTimeoutMs, uptime);

        // Heartbeat arrives before sequence: board is kElectricallyOff → no-op
        controller.OnPeerHeartbeat(1, DeviceState::kInitializing);
        REQUIRE(controller.board_state(1) == AdcBoardState::kElectricallyOff);

        controller.StartPowerSequence();
        REQUIRE(controller.board_state(1) == AdcBoardState::kElectricallyOn);

        // Next heartbeat triggers transition and sequence advancement
        controller.OnPeerHeartbeat(1, DeviceState::kInitializing);

        REQUIRE(controller.board_state(1) == AdcBoardState::kInitializing);
        REQUIRE(controller.board_state(2) == AdcBoardState::kElectricallyOn);
      }
    }
  }

  SECTION("The CheckSequenceTimeout() method") {
    SECTION("When no sequence has started") {
      SECTION("Should be a no-op") {
        RecordingMessageSender sender;
        RecordingPowerSwitch power_switch;
        StubUptimeProvider uptime;
        Controller controller(sender, power_switch, kPowerOnTimeoutMs, uptime);

        uptime.set_uptime_ms(kPowerOnTimeoutMs + 1);
        controller.CheckSequenceTimeout();

        REQUIRE(power_switch.power_on_calls().empty());
      }
    }

    SECTION("When current board does not respond before timeout") {
      SECTION("Should mark board unresponsive and power on next board") {
        RecordingMessageSender sender;
        RecordingPowerSwitch power_switch;
        StubUptimeProvider uptime;
        Controller controller(sender, power_switch, kPowerOnTimeoutMs, uptime);

        uptime.set_uptime_ms(0);
        controller.StartPowerSequence();
        uptime.set_uptime_ms(kPowerOnTimeoutMs + 1);
        controller.CheckSequenceTimeout();

        REQUIRE(controller.board_state(1) == AdcBoardState::kUnresponsive);
        REQUIRE(controller.board_state(2) == AdcBoardState::kElectricallyOn);
        REQUIRE(power_switch.power_on_calls().size() == 2);
        REQUIRE(power_switch.power_on_calls().at(1) == 2);
      }
    }

    SECTION("When all boards have timed out") {
      SECTION("Should complete sequence and ignore extra timeout checks") {
        RecordingMessageSender sender;
        RecordingPowerSwitch power_switch;
        StubUptimeProvider uptime;
        ControllerMax controller(sender, power_switch, kPowerOnTimeoutMs, uptime);

        uptime.set_uptime_ms(0);
        controller.StartPowerSequence();
        for (std::uint8_t peer_id = 1; peer_id <= kMaxBoardCount; ++peer_id) {
          uptime.set_uptime_ms(peer_id * (kPowerOnTimeoutMs + 1));
          controller.CheckSequenceTimeout();
        }

        const auto power_on_count_after_completion = power_switch.power_on_count();
        uptime.set_uptime_ms((kMaxBoardCount + 1) * (kPowerOnTimeoutMs + 1));
        controller.CheckSequenceTimeout();

        REQUIRE(controller.board_state(kMaxBoardCount) == AdcBoardState::kUnresponsive);
        REQUIRE(power_switch.power_on_count() == power_on_count_after_completion);
      }
    }
  }

  SECTION("The StopAll() method") {
    SECTION("When multiple boards have been powered on") {
      SECTION("Should power off all boards") {
        RecordingMessageSender sender;
        RecordingPowerSwitch power_switch;
        StubUptimeProvider uptime;
        Controller controller(sender, power_switch, kPowerOnTimeoutMs, uptime);

        uptime.set_uptime_ms(0);
        controller.StartPowerSequence();
        uptime.set_uptime_ms(kPowerOnTimeoutMs + 1);
        controller.CheckSequenceTimeout();
        controller.StopAll();

        for (std::uint8_t id = 1; id <= kBoardCount; ++id) {
          REQUIRE(controller.board_state(id) == AdcBoardState::kElectricallyOff);
        }
        REQUIRE(power_switch.power_off_calls().size() == kBoardCount);
      }
    }

    SECTION("When called before a new startup sequence") {
      SECTION("Should allow a new sequence to start again from board 1") {
        RecordingMessageSender sender;
        RecordingPowerSwitch power_switch;
        StubUptimeProvider uptime;
        Controller controller(sender, power_switch, kPowerOnTimeoutMs, uptime);

        controller.StartPowerSequence();
        controller.StopAll();
        const auto power_on_count_before = power_switch.power_on_count();

        controller.StartPowerSequence();

        REQUIRE(power_switch.power_on_count() == power_on_count_before + 1);
        REQUIRE(controller.board_state(1) == AdcBoardState::kElectricallyOn);
      }
    }
  }

  SECTION("The peer recovery behavior") {
    SECTION("When a previously unresponsive board sends a late heartbeat") {
      SECTION("Should move board state to kInitializing and emit heartbeat") {
        RecordingMessageSender sender;
        RecordingPowerSwitch power_switch;
        StubUptimeProvider uptime;
        Controller controller(sender, power_switch, kPowerOnTimeoutMs, uptime);

        uptime.set_uptime_ms(0);
        controller.StartPowerSequence();
        uptime.set_uptime_ms(kPowerOnTimeoutMs + 1);
        controller.CheckSequenceTimeout();
        sender.reset_heartbeat_count();

        controller.OnPeerHeartbeat(1, DeviceState::kInitializing);

        REQUIRE(controller.board_state(1) == AdcBoardState::kInitializing);
        REQUIRE(sender.heartbeat_count() == 1);
      }
    }
  }

  SECTION("The invalid id handling") {
    SECTION("When invalid ids are passed to PowerOn() and PowerOff()") {
      SECTION("Should keep states unchanged and not call power switch") {
        RecordingMessageSender sender;
        RecordingPowerSwitch power_switch;
        StubUptimeProvider uptime;
        ControllerMax controller(sender, power_switch, kPowerOnTimeoutMs, uptime);

        controller.PowerOn(0);
        controller.PowerOff(0);
        controller.PowerOn(kMaxBoardCount + 1);
        controller.PowerOff(kMaxBoardCount + 1);

        REQUIRE(power_switch.power_on_calls().empty());
        REQUIRE(power_switch.power_off_calls().empty());
        REQUIRE(controller.board_state(1) == AdcBoardState::kElectricallyOff);
      }
    }

    SECTION("When invalid ids are passed to board_state()") {
      SECTION("Should return kElectricallyOff") {
        RecordingMessageSender sender;
        RecordingPowerSwitch power_switch;
        StubUptimeProvider uptime;
        ControllerMax controller(sender, power_switch, kPowerOnTimeoutMs, uptime);

        REQUIRE(controller.board_state(0) == AdcBoardState::kElectricallyOff);
        REQUIRE(controller.board_state(kMaxBoardCount + 1) == AdcBoardState::kElectricallyOff);
      }
    }
  }
}

#endif

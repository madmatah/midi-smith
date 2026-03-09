#if defined(UNIT_TESTS)

#include "domain/adc/adc_boards_controller.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <optional>
#include <vector>

#include "domain/adc/adc_board_power_switch_requirements.hpp"
#include "domain/adc/adc_board_state.hpp"
#include "os-types/uptime_provider_requirements.hpp"
#include "protocol/peer_status.hpp"

namespace {

class RecordingPowerSwitch final
    : public midismith::main_board::domain::adc::AdcBoardPowerSwitchRequirements {
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
  void set_uptime_ms(std::uint32_t ms) noexcept {
    uptime_ms_ = ms;
  }

 private:
  std::uint32_t uptime_ms_ = 0;
};

constexpr std::uint32_t kPowerOnTimeoutMs = 1000;
constexpr std::size_t kBoardCount = 3;

using Controller = midismith::main_board::domain::adc::AdcBoardsController<kBoardCount>;
using AdcBoardState = midismith::main_board::domain::adc::AdcBoardState;
using PeerStatus = midismith::protocol::PeerStatus;
using PeerConnectivity = midismith::protocol::PeerConnectivity;

PeerStatus HealthyStatus() noexcept {
  return PeerStatus{.connectivity = PeerConnectivity::kHealthy};
}

PeerStatus LostStatus() noexcept {
  return PeerStatus{.connectivity = PeerConnectivity::kLost};
}

}  // namespace

TEST_CASE("AdcBoardsController — initial state") {
  RecordingPowerSwitch power_switch;
  StubUptimeProvider uptime;
  Controller controller(power_switch, kPowerOnTimeoutMs, uptime);

  SECTION("All boards should start in kElectricallyOff") {
    for (std::uint8_t id = 1; id <= kBoardCount; ++id) {
      REQUIRE(controller.board_state(id) == AdcBoardState::kElectricallyOff);
    }
  }

  SECTION("board_state with invalid peer id 0 should return kElectricallyOff") {
    REQUIRE(controller.board_state(0) == AdcBoardState::kElectricallyOff);
  }

  SECTION("board_state with peer id above board count should return kElectricallyOff") {
    REQUIRE(controller.board_state(kBoardCount + 1) == AdcBoardState::kElectricallyOff);
  }
}

TEST_CASE("AdcBoardsController — PowerOn() / PowerOff()") {
  RecordingPowerSwitch power_switch;
  StubUptimeProvider uptime;
  Controller controller(power_switch, kPowerOnTimeoutMs, uptime);

  SECTION("PowerOn() should set board to kElectricallyOn and call power switch") {
    controller.PowerOn(1);
    REQUIRE(controller.board_state(1) == AdcBoardState::kElectricallyOn);
    REQUIRE(power_switch.power_on_calls().size() == 1);
    REQUIRE(power_switch.power_on_calls()[0] == 1);
  }

  SECTION("PowerOff() from kElectricallyOn should set board to kElectricallyOff") {
    controller.PowerOn(1);
    controller.PowerOff(1);
    REQUIRE(controller.board_state(1) == AdcBoardState::kElectricallyOff);
    REQUIRE(power_switch.power_off_calls().size() == 1);
    REQUIRE(power_switch.power_off_calls()[0] == 1);
  }

  SECTION("PowerOn() with invalid peer id should be a no-op") {
    controller.PowerOn(0);
    controller.PowerOn(kBoardCount + 1);
    REQUIRE(power_switch.power_on_calls().empty());
  }

  SECTION("PowerOff() with invalid peer id should be a no-op") {
    controller.PowerOff(0);
    controller.PowerOff(kBoardCount + 1);
    REQUIRE(power_switch.power_off_calls().empty());
  }
}

TEST_CASE("AdcBoardsController — OnPeerChanged()") {
  RecordingPowerSwitch power_switch;
  StubUptimeProvider uptime;
  Controller controller(power_switch, kPowerOnTimeoutMs, uptime);

  SECTION("kHealthy when board is kElectricallyOn should set it to kReachable") {
    controller.PowerOn(1);
    controller.OnPeerChanged(1, HealthyStatus());
    REQUIRE(controller.board_state(1) == AdcBoardState::kReachable);
  }

  SECTION("kLost when board is kReachable should revert to kElectricallyOn") {
    controller.PowerOn(1);
    controller.OnPeerChanged(1, HealthyStatus());
    controller.OnPeerChanged(1, LostStatus());
    REQUIRE(controller.board_state(1) == AdcBoardState::kElectricallyOn);
  }

  SECTION("OnPeerChanged with invalid peer_id 0 should not crash") {
    controller.OnPeerChanged(0, HealthyStatus());
  }

  SECTION("OnPeerChanged with peer_id above board count should not crash") {
    controller.OnPeerChanged(kBoardCount + 1, HealthyStatus());
  }
}

TEST_CASE("AdcBoardsController — StartPowerSequence()") {
  RecordingPowerSwitch power_switch;
  StubUptimeProvider uptime;
  Controller controller(power_switch, kPowerOnTimeoutMs, uptime);

  SECTION("Should power on board 1 and call power switch") {
    controller.StartPowerSequence();
    REQUIRE(controller.board_state(1) == AdcBoardState::kElectricallyOn);
    REQUIRE(power_switch.power_on_calls().size() == 1);
    REQUIRE(power_switch.power_on_calls()[0] == 1);
  }

  SECTION("Second call should be ignored (sequence already started)") {
    controller.StartPowerSequence();
    controller.StartPowerSequence();
    REQUIRE(power_switch.power_on_count() == 1);
  }
}

TEST_CASE("AdcBoardsController — CheckSequenceTimeout() before StartPowerSequence()") {
  RecordingPowerSwitch power_switch;
  StubUptimeProvider uptime;
  Controller controller(power_switch, kPowerOnTimeoutMs, uptime);

  SECTION("Should be a no-op") {
    uptime.set_uptime_ms(kPowerOnTimeoutMs + 1);
    controller.CheckSequenceTimeout();
    REQUIRE(power_switch.power_on_calls().empty());
  }
}

TEST_CASE("AdcBoardsController — sequence advances on timeout") {
  RecordingPowerSwitch power_switch;
  StubUptimeProvider uptime;
  Controller controller(power_switch, kPowerOnTimeoutMs, uptime);

  uptime.set_uptime_ms(0);
  controller.StartPowerSequence();

  SECTION("When board 1 does not respond within timeout") {
    SECTION("Should mark board 1 unresponsive and power on board 2") {
      uptime.set_uptime_ms(kPowerOnTimeoutMs + 1);
      controller.CheckSequenceTimeout();

      REQUIRE(controller.board_state(1) == AdcBoardState::kUnresponsive);
      REQUIRE(controller.board_state(2) == AdcBoardState::kElectricallyOn);
      REQUIRE(power_switch.power_on_calls().size() == 2);
      REQUIRE(power_switch.power_on_calls()[1] == 2);
    }
  }

  SECTION("When board 1 becomes reachable before timeout") {
    SECTION("Should advance immediately to board 2 via OnPeerChanged") {
      controller.OnPeerChanged(1, HealthyStatus());

      REQUIRE(controller.board_state(1) == AdcBoardState::kReachable);
      REQUIRE(controller.board_state(2) == AdcBoardState::kElectricallyOn);
    }
  }
}

TEST_CASE("AdcBoardsController — sequence completes after all boards") {
  RecordingPowerSwitch power_switch;
  StubUptimeProvider uptime;
  Controller controller(power_switch, kPowerOnTimeoutMs, uptime);

  uptime.set_uptime_ms(0);
  controller.StartPowerSequence();

  // Advance through all boards via timeout
  for (std::uint8_t i = 0; i < kBoardCount; ++i) {
    uptime.set_uptime_ms(kPowerOnTimeoutMs + 1);
    controller.CheckSequenceTimeout();
  }

  SECTION("Should not call PowerOn again after sequence is complete") {
    const auto count_after_sequence = power_switch.power_on_count();
    uptime.set_uptime_ms(2 * kPowerOnTimeoutMs + 1);
    controller.CheckSequenceTimeout();
    REQUIRE(power_switch.power_on_count() == count_after_sequence);
  }
}

TEST_CASE("AdcBoardsController — StopAll()") {
  RecordingPowerSwitch power_switch;
  StubUptimeProvider uptime;
  Controller controller(power_switch, kPowerOnTimeoutMs, uptime);

  uptime.set_uptime_ms(0);
  controller.StartPowerSequence();

  // Advance to board 2 via timeout
  uptime.set_uptime_ms(kPowerOnTimeoutMs + 1);
  controller.CheckSequenceTimeout();

  SECTION("Should power off all boards and call power switch for each") {
    controller.StopAll();

    for (std::uint8_t id = 1; id <= kBoardCount; ++id) {
      REQUIRE(controller.board_state(id) == AdcBoardState::kElectricallyOff);
    }
    REQUIRE(power_switch.power_off_calls().size() == kBoardCount);
  }

  SECTION("Should allow StartPowerSequence to be called again") {
    controller.StopAll();
    const auto power_on_count_before = power_switch.power_on_count();

    controller.StartPowerSequence();

    REQUIRE(power_switch.power_on_count() == power_on_count_before + 1);
    REQUIRE(controller.board_state(1) == AdcBoardState::kElectricallyOn);
  }
}

TEST_CASE("AdcBoardsController — kUnresponsive board can recover to kReachable") {
  RecordingPowerSwitch power_switch;
  StubUptimeProvider uptime;
  Controller controller(power_switch, kPowerOnTimeoutMs, uptime);

  uptime.set_uptime_ms(0);
  controller.StartPowerSequence();

  // Timeout: board 1 becomes kUnresponsive, board 2 is powered on
  uptime.set_uptime_ms(kPowerOnTimeoutMs + 1);
  controller.CheckSequenceTimeout();

  SECTION("A late heartbeat from board 1 should move it to kReachable") {
    controller.OnPeerChanged(1, HealthyStatus());
    REQUIRE(controller.board_state(1) == AdcBoardState::kReachable);
  }
}

#endif

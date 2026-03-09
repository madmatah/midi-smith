#if defined(UNIT_TESTS)

#include "app/adc/adc_board_controller.hpp"

#include <catch2/catch_test_macros.hpp>

#include "app/adc/adc_board_state.hpp"
#include "app/messaging/main_board_message_sender_requirements.hpp"
#include "protocol/messages.hpp"

namespace {

class RecordingMessageSender final
    : public midismith::main_board::app::messaging::MainBoardMessageSenderRequirements {
 public:
  bool SendHeartbeat(midismith::protocol::DeviceState /*state*/) noexcept override {
    ++heartbeat_count_;
    return true;
  }
  bool SendStartAdc(std::uint8_t /*target_node_id*/) noexcept override {
    return true;
  }
  bool SendStopAdc(std::uint8_t /*target_node_id*/) noexcept override {
    return true;
  }
  bool SendStartCalibration(std::uint8_t /*target_node_id*/,
                            midismith::protocol::CalibMode /*mode*/) noexcept override {
    return true;
  }

  [[nodiscard]] int heartbeat_count() const noexcept {
    return heartbeat_count_;
  }

 private:
  int heartbeat_count_{0};
};

}  // namespace

using midismith::main_board::app::adc::AdcBoardController;
using midismith::main_board::app::adc::AdcBoardState;

TEST_CASE("AdcBoardController — initial state") {
  RecordingMessageSender sender;
  AdcBoardController controller{sender};

  SECTION("Should start in kElectricallyOff") {
    REQUIRE(controller.state() == AdcBoardState::kElectricallyOff);
  }
}

TEST_CASE("AdcBoardController — PowerOn()") {
  RecordingMessageSender sender;
  AdcBoardController controller{sender};

  SECTION("When in kElectricallyOff") {
    SECTION("Should transition to kElectricallyOn") {
      controller.PowerOn();
      REQUIRE(controller.state() == AdcBoardState::kElectricallyOn);
    }
  }

  SECTION("When already in kElectricallyOn") {
    SECTION("Should be a no-op") {
      controller.PowerOn();
      controller.PowerOn();
      REQUIRE(controller.state() == AdcBoardState::kElectricallyOn);
    }
  }

  SECTION("When in kReachable") {
    SECTION("Should be a no-op") {
      controller.PowerOn();
      controller.OnReachable();
      controller.PowerOn();
      REQUIRE(controller.state() == AdcBoardState::kReachable);
    }
  }
}

TEST_CASE("AdcBoardController — PowerOff()") {
  RecordingMessageSender sender;
  AdcBoardController controller{sender};

  SECTION("When in kElectricallyOn") {
    SECTION("Should transition to kElectricallyOff") {
      controller.PowerOn();
      controller.PowerOff();
      REQUIRE(controller.state() == AdcBoardState::kElectricallyOff);
    }
  }

  SECTION("When in kReachable") {
    SECTION("Should transition to kElectricallyOff") {
      controller.PowerOn();
      controller.OnReachable();
      controller.PowerOff();
      REQUIRE(controller.state() == AdcBoardState::kElectricallyOff);
    }
  }

  SECTION("When in kUnresponsive") {
    SECTION("Should transition to kElectricallyOff") {
      controller.PowerOn();
      controller.MarkUnresponsive();
      controller.PowerOff();
      REQUIRE(controller.state() == AdcBoardState::kElectricallyOff);
    }
  }
}

TEST_CASE("AdcBoardController — MarkUnresponsive()") {
  RecordingMessageSender sender;
  AdcBoardController controller{sender};

  SECTION("When in kElectricallyOn") {
    SECTION("Should transition to kUnresponsive") {
      controller.PowerOn();
      controller.MarkUnresponsive();
      REQUIRE(controller.state() == AdcBoardState::kUnresponsive);
    }
  }

  SECTION("When in kElectricallyOff") {
    SECTION("Should be a no-op") {
      controller.MarkUnresponsive();
      REQUIRE(controller.state() == AdcBoardState::kElectricallyOff);
    }
  }

  SECTION("When in kReachable") {
    SECTION("Should be a no-op") {
      controller.PowerOn();
      controller.OnReachable();
      controller.MarkUnresponsive();
      REQUIRE(controller.state() == AdcBoardState::kReachable);
    }
  }
}

TEST_CASE("AdcBoardController — OnReachable()") {
  RecordingMessageSender sender;
  AdcBoardController controller{sender};

  SECTION("When in kElectricallyOn") {
    controller.PowerOn();

    SECTION("Should transition to kReachable") {
      controller.OnReachable();
      REQUIRE(controller.state() == AdcBoardState::kReachable);
    }

    SECTION("Should send a heartbeat") {
      controller.OnReachable();
      REQUIRE(sender.heartbeat_count() == 1);
    }
  }

  SECTION("When in kUnresponsive") {
    controller.PowerOn();
    controller.MarkUnresponsive();

    SECTION("Should transition to kReachable (recovery)") {
      controller.OnReachable();
      REQUIRE(controller.state() == AdcBoardState::kReachable);
    }

    SECTION("Should send a heartbeat on recovery") {
      controller.OnReachable();
      REQUIRE(sender.heartbeat_count() == 1);
    }
  }

  SECTION("When in kElectricallyOff") {
    SECTION("Should be a no-op") {
      controller.OnReachable();
      REQUIRE(controller.state() == AdcBoardState::kElectricallyOff);
    }

    SECTION("Should not send a heartbeat") {
      controller.OnReachable();
      REQUIRE(sender.heartbeat_count() == 0);
    }
  }

  SECTION("When already in kReachable") {
    controller.PowerOn();
    controller.OnReachable();

    SECTION("Should be idempotent") {
      controller.OnReachable();
      REQUIRE(controller.state() == AdcBoardState::kReachable);
    }

    SECTION("Should not send a second heartbeat") {
      controller.OnReachable();
      REQUIRE(sender.heartbeat_count() == 1);
    }
  }
}

TEST_CASE("AdcBoardController — OnLost()") {
  RecordingMessageSender sender;
  AdcBoardController controller{sender};

  SECTION("When in kReachable") {
    SECTION("Should transition to kElectricallyOn") {
      controller.PowerOn();
      controller.OnReachable();
      controller.OnLost();
      REQUIRE(controller.state() == AdcBoardState::kElectricallyOn);
    }
  }

  SECTION("When in kElectricallyOn") {
    SECTION("Should be a no-op") {
      controller.PowerOn();
      controller.OnLost();
      REQUIRE(controller.state() == AdcBoardState::kElectricallyOn);
    }
  }

  SECTION("When in kElectricallyOff") {
    SECTION("Should be a no-op") {
      controller.OnLost();
      REQUIRE(controller.state() == AdcBoardState::kElectricallyOff);
    }
  }

  SECTION("When in kUnresponsive") {
    SECTION("Should be a no-op") {
      controller.PowerOn();
      controller.MarkUnresponsive();
      controller.OnLost();
      REQUIRE(controller.state() == AdcBoardState::kUnresponsive);
    }
  }
}

#endif

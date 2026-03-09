#if defined(UNIT_TESTS)

#include "app/adc/adc_board_controller.hpp"

#include <catch2/catch_test_macros.hpp>
#include <vector>

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
  bool SendStartAdc(std::uint8_t target_node_id) noexcept override {
    start_adc_calls_.push_back(target_node_id);
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

  [[nodiscard]] const std::vector<std::uint8_t>& start_adc_calls() const noexcept {
    return start_adc_calls_;
  }

 private:
  int heartbeat_count_{0};
  std::vector<std::uint8_t> start_adc_calls_;
};

}  // namespace

using midismith::main_board::app::adc::AdcBoardController;
using midismith::main_board::app::adc::AdcBoardState;
using midismith::protocol::DeviceState;

TEST_CASE("The AdcBoardController class") {
  RecordingMessageSender sender;
  AdcBoardController controller{sender, 1};

  SECTION("The state() method") {
    SECTION("When the controller is newly created") {
      SECTION("Should be kElectricallyOff") {
        REQUIRE(controller.state() == AdcBoardState::kElectricallyOff);
      }
    }
  }

  SECTION("The PowerOn() method") {
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

    SECTION("When in a connected state") {
      SECTION("Should be a no-op") {
        controller.PowerOn();
        controller.OnPeerHealthy(DeviceState::kInitializing);
        controller.PowerOn();
        REQUIRE(controller.state() == AdcBoardState::kInitializing);
      }
    }
  }

  SECTION("The PowerOff() method") {
    SECTION("When in kElectricallyOn") {
      SECTION("Should transition to kElectricallyOff") {
        controller.PowerOn();
        controller.PowerOff();
        REQUIRE(controller.state() == AdcBoardState::kElectricallyOff);
      }
    }

    SECTION("When in a connected state") {
      SECTION("Should transition to kElectricallyOff") {
        controller.PowerOn();
        controller.OnPeerHealthy(DeviceState::kInitializing);
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

  SECTION("The MarkUnresponsive() method") {
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

    SECTION("When in a connected state") {
      SECTION("Should be a no-op") {
        controller.PowerOn();
        controller.OnPeerHealthy(DeviceState::kInitializing);
        controller.MarkUnresponsive();
        REQUIRE(controller.state() == AdcBoardState::kInitializing);
      }
    }
  }

  SECTION("The OnPeerHealthy() method") {
    SECTION("When in kElectricallyOff") {
      SECTION("Should be a no-op") {
        controller.OnPeerHealthy(DeviceState::kInitializing);
        REQUIRE(controller.state() == AdcBoardState::kElectricallyOff);
        REQUIRE(sender.heartbeat_count() == 0);
      }
    }

    SECTION("When in kElectricallyOn and the peer reports kInitializing") {
      SECTION("Should transition to kInitializing and send a heartbeat") {
        controller.PowerOn();
        controller.OnPeerHealthy(DeviceState::kInitializing);
        REQUIRE(controller.state() == AdcBoardState::kInitializing);
        REQUIRE(sender.heartbeat_count() == 1);
        REQUIRE(sender.start_adc_calls().empty());
      }
    }

    SECTION("When in kElectricallyOn and the peer reports kReady") {
      SECTION("Should transition to kReady, send a heartbeat, and send StartAdc") {
        controller.PowerOn();
        controller.OnPeerHealthy(DeviceState::kReady);
        REQUIRE(controller.state() == AdcBoardState::kReady);
        REQUIRE(sender.heartbeat_count() == 1);
        REQUIRE(sender.start_adc_calls().size() == 1);
        REQUIRE(sender.start_adc_calls().at(0) == 1);
      }
    }

    SECTION("When in kElectricallyOn and the peer reports kRunning") {
      SECTION("Should transition to kAcquiring and send a heartbeat") {
        controller.PowerOn();
        controller.OnPeerHealthy(DeviceState::kRunning);
        REQUIRE(controller.state() == AdcBoardState::kAcquiring);
        REQUIRE(sender.heartbeat_count() == 1);
        REQUIRE(sender.start_adc_calls().empty());
      }
    }

    SECTION("When in kUnresponsive and the peer reports kInitializing") {
      SECTION("Should recover to kInitializing and send a heartbeat") {
        controller.PowerOn();
        controller.MarkUnresponsive();
        controller.OnPeerHealthy(DeviceState::kInitializing);
        REQUIRE(controller.state() == AdcBoardState::kInitializing);
        REQUIRE(sender.heartbeat_count() == 1);
      }
    }

    SECTION("When in kUnresponsive and the peer reports kReady") {
      SECTION("Should recover to kReady, send a heartbeat, and send StartAdc") {
        controller.PowerOn();
        controller.MarkUnresponsive();
        controller.OnPeerHealthy(DeviceState::kReady);
        REQUIRE(controller.state() == AdcBoardState::kReady);
        REQUIRE(sender.heartbeat_count() == 1);
        REQUIRE(sender.start_adc_calls().size() == 1);
      }
    }

    SECTION("When in kInitializing and the peer reports kReady") {
      SECTION("Should transition to kReady and send StartAdc without a new heartbeat") {
        controller.PowerOn();
        controller.OnPeerHealthy(DeviceState::kInitializing);
        const int heartbeats_before = sender.heartbeat_count();

        controller.OnPeerHealthy(DeviceState::kReady);

        REQUIRE(controller.state() == AdcBoardState::kReady);
        REQUIRE(sender.heartbeat_count() == heartbeats_before);
        REQUIRE(sender.start_adc_calls().size() == 1);
      }
    }

    SECTION("When in kInitializing and the peer reports kRunning") {
      SECTION("Should transition to kAcquiring") {
        controller.PowerOn();
        controller.OnPeerHealthy(DeviceState::kInitializing);
        controller.OnPeerHealthy(DeviceState::kRunning);
        REQUIRE(controller.state() == AdcBoardState::kAcquiring);
      }
    }

    SECTION("When in kReady and the peer reports kRunning") {
      SECTION("Should transition to kAcquiring") {
        controller.PowerOn();
        controller.OnPeerHealthy(DeviceState::kReady);
        controller.OnPeerHealthy(DeviceState::kRunning);
        REQUIRE(controller.state() == AdcBoardState::kAcquiring);
      }
    }

    SECTION("When the peer reports the same state twice") {
      SECTION("Should not send a duplicate StartAdc") {
        controller.PowerOn();
        controller.OnPeerHealthy(DeviceState::kReady);
        const auto start_adc_count_after_first = sender.start_adc_calls().size();

        controller.OnPeerHealthy(DeviceState::kReady);

        REQUIRE(sender.start_adc_calls().size() == start_adc_count_after_first);
      }
    }
  }

  SECTION("The OnLost() method") {
    SECTION("When in kInitializing") {
      SECTION("Should transition to kElectricallyOn") {
        controller.PowerOn();
        controller.OnPeerHealthy(DeviceState::kInitializing);
        controller.OnLost();
        REQUIRE(controller.state() == AdcBoardState::kElectricallyOn);
      }
    }

    SECTION("When in kReady") {
      SECTION("Should transition to kElectricallyOn") {
        controller.PowerOn();
        controller.OnPeerHealthy(DeviceState::kReady);
        controller.OnLost();
        REQUIRE(controller.state() == AdcBoardState::kElectricallyOn);
      }
    }

    SECTION("When in kAcquiring") {
      SECTION("Should transition to kElectricallyOn") {
        controller.PowerOn();
        controller.OnPeerHealthy(DeviceState::kRunning);
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
}

#endif

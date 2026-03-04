#if defined(UNIT_TESTS)

#include "app/messaging/main_board_can_message_sender.hpp"

#include <array>
#include <catch2/catch_test_macros.hpp>
#include <optional>

#include "bsp-types/can/fdcan_frame.hpp"
#include "bsp-types/can/fdcan_transceiver_requirements.hpp"
#include "protocol-can/can_mapper.hpp"
#include "protocol/builders.hpp"
#include "protocol/messages.hpp"

namespace {

class RecordingTransceiver final : public midismith::bsp::can::FdcanTransceiverRequirements {
 public:
  bool Transmit(const midismith::bsp::can::FdcanFrame& frame) noexcept override {
    last_frame_ = frame;
    return true;
  }

  [[nodiscard]] const std::optional<midismith::bsp::can::FdcanFrame>& last_frame() const noexcept {
    return last_frame_;
  }

 private:
  std::optional<midismith::bsp::can::FdcanFrame> last_frame_;
};

}  // namespace

using midismith::main_board::app::messaging::MainBoardCanMessageSender;
using midismith::protocol::CalibMode;
using midismith::protocol::DeviceState;
using midismith::protocol::MainBoardMessageBuilder;

TEST_CASE("The MainBoardCanMessageSender class") {
  RecordingTransceiver transceiver;
  MainBoardCanMessageSender sender(transceiver);

  SECTION("The SendHeartbeat() method") {
    SECTION("Should transmit a frame with the correct CAN identifier") {
      sender.SendHeartbeat(DeviceState::kRunning);

      const auto expected_id = midismith::protocol_can::CanIdentifierMapper::EncodeId(
          MainBoardMessageBuilder().BuildHeartbeat(DeviceState::kRunning).first);
      REQUIRE(transceiver.last_frame().has_value());
      REQUIRE(transceiver.last_frame()->identifier == expected_id);
    }

    SECTION("Should transmit a frame with DLC = 1") {
      sender.SendHeartbeat(DeviceState::kRunning);

      REQUIRE(transceiver.last_frame().has_value());
      REQUIRE(transceiver.last_frame()->data_length_bytes ==
              midismith::protocol::Heartbeat::kSerializedSizeBytes);
    }

    SECTION("Should encode the device state as the single payload byte") {
      sender.SendHeartbeat(DeviceState::kIdle);

      REQUIRE(transceiver.last_frame().has_value());
      REQUIRE(transceiver.last_frame()->data[0] == static_cast<std::uint8_t>(DeviceState::kIdle));
    }
  }

  SECTION("The SendStartAdc() method") {
    SECTION("When called with a target node id") {
      sender.SendStartAdc(3);

      SECTION("Should transmit a frame with the correct CAN identifier") {
        const auto expected_id = midismith::protocol_can::CanIdentifierMapper::EncodeId(
            MainBoardMessageBuilder().BuildStartAdc(3).first);
        REQUIRE(transceiver.last_frame().has_value());
        REQUIRE(transceiver.last_frame()->identifier == expected_id);
      }

      SECTION("Should transmit a frame with DLC = 1") {
        REQUIRE(transceiver.last_frame().has_value());
        REQUIRE(transceiver.last_frame()->data_length_bytes ==
                midismith::protocol::AdcStart::kSerializedSizeBytes);
      }
    }
  }

  SECTION("The SendStopAdc() method") {
    SECTION("When called with a target node id") {
      sender.SendStopAdc(5);

      SECTION("Should transmit a frame with the correct CAN identifier") {
        const auto expected_id = midismith::protocol_can::CanIdentifierMapper::EncodeId(
            MainBoardMessageBuilder().BuildStopAdc(5).first);
        REQUIRE(transceiver.last_frame().has_value());
        REQUIRE(transceiver.last_frame()->identifier == expected_id);
      }

      SECTION("Should transmit a frame with DLC = 1") {
        REQUIRE(transceiver.last_frame().has_value());
        REQUIRE(transceiver.last_frame()->data_length_bytes ==
                midismith::protocol::AdcStop::kSerializedSizeBytes);
      }
    }
  }

  SECTION("The SendStartCalibration() method") {
    SECTION("When called with CalibMode::kAuto") {
      sender.SendStartCalibration(2, CalibMode::kAuto);

      SECTION("Should transmit a frame with the correct CAN identifier") {
        const auto expected_id = midismith::protocol_can::CanIdentifierMapper::EncodeId(
            MainBoardMessageBuilder().BuildStartCalibration(2, CalibMode::kAuto).first);
        REQUIRE(transceiver.last_frame().has_value());
        REQUIRE(transceiver.last_frame()->identifier == expected_id);
      }

      SECTION("Should transmit a frame with DLC = 2") {
        REQUIRE(transceiver.last_frame().has_value());
        REQUIRE(transceiver.last_frame()->data_length_bytes ==
                midismith::protocol::CalibStart::kSerializedSizeBytes);
      }

      SECTION("Should encode kAuto in the second payload byte") {
        REQUIRE(transceiver.last_frame().has_value());
        REQUIRE(transceiver.last_frame()->data[1] == static_cast<std::uint8_t>(CalibMode::kAuto));
      }
    }

    SECTION("When called with CalibMode::kManual") {
      sender.SendStartCalibration(2, CalibMode::kManual);

      SECTION("Should encode kManual in the second payload byte") {
        REQUIRE(transceiver.last_frame().has_value());
        REQUIRE(transceiver.last_frame()->data[1] == static_cast<std::uint8_t>(CalibMode::kManual));
      }
    }
  }
}

#endif

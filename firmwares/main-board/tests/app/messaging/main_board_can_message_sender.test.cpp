#if defined(UNIT_TESTS)

#include "app/messaging/main_board_can_message_sender.hpp"

#include <array>
#include <catch2/catch_test_macros.hpp>
#include <fakeit.hpp>

#include "bsp-types/can/fdcan_frame.hpp"
#include "bsp-types/can/fdcan_transceiver_requirements.hpp"
#include "protocol-can/can_mapper.hpp"
#include "protocol/builders.hpp"
#include "protocol/messages.hpp"

namespace {

using fakeit::Mock;
using fakeit::Verify;
using fakeit::When;

#define fakeit_Method(mock, method) Method(mock, method)

}  // namespace

using midismith::main_board::app::messaging::MainBoardCanMessageSender;
using midismith::protocol::CalibMode;
using midismith::protocol::DataSegmentAckStatus;
using midismith::protocol::DeviceState;
using midismith::protocol::MainBoardMessageBuilder;

TEST_CASE("The MainBoardCanMessageSender class") {
  Mock<midismith::bsp::can::FdcanTransceiverRequirements> transceiver_mock;
  MainBoardCanMessageSender sender(transceiver_mock.get());
  midismith::bsp::can::FdcanFrame captured_frame{};

  auto capture_frame = [&](const midismith::bsp::can::FdcanFrame& f) {
    captured_frame = f;
    return true;
  };

  SECTION("The SendHeartbeat() method") {
    SECTION("Should transmit a frame with the correct CAN identifier") {
      const auto expected_id = midismith::protocol_can::CanIdentifierMapper::EncodeId(
          MainBoardMessageBuilder().BuildHeartbeat(DeviceState::kRunning).first);

      When(fakeit_Method(transceiver_mock, Transmit)).Do(capture_frame);

      sender.SendHeartbeat(DeviceState::kRunning);

      Verify(fakeit_Method(transceiver_mock, Transmit)).Once();
      REQUIRE(captured_frame.identifier == expected_id);
    }

    SECTION("Should transmit a frame with DLC = 1") {
      When(fakeit_Method(transceiver_mock, Transmit)).Do(capture_frame);

      sender.SendHeartbeat(DeviceState::kRunning);

      Verify(fakeit_Method(transceiver_mock, Transmit)).Once();
      REQUIRE(captured_frame.data_length_bytes ==
              midismith::protocol::Heartbeat::kSerializedSizeBytes);
    }

    SECTION("Should encode the device state as the single payload byte") {
      When(fakeit_Method(transceiver_mock, Transmit)).Do(capture_frame);

      sender.SendHeartbeat(DeviceState::kReady);

      Verify(fakeit_Method(transceiver_mock, Transmit)).Once();
      REQUIRE(captured_frame.data[0] == static_cast<std::uint8_t>(DeviceState::kReady));
    }
  }

  SECTION("The SendStartAdc() method") {
    SECTION("When called with a target node id") {
      const auto expected_id = midismith::protocol_can::CanIdentifierMapper::EncodeId(
          MainBoardMessageBuilder().BuildStartAdc(3).first);

      When(fakeit_Method(transceiver_mock, Transmit)).Do(capture_frame);

      sender.SendStartAdc(3);

      SECTION("Should transmit a frame with the correct CAN identifier") {
        Verify(fakeit_Method(transceiver_mock, Transmit)).AtLeastOnce();
        REQUIRE(captured_frame.identifier == expected_id);
      }

      SECTION("Should transmit a frame with DLC = 1") {
        Verify(fakeit_Method(transceiver_mock, Transmit)).AtLeastOnce();
        REQUIRE(captured_frame.data_length_bytes ==
                midismith::protocol::AdcStart::kSerializedSizeBytes);
      }
    }
  }

  SECTION("The SendStopAdc() method") {
    SECTION("When called with a target node id") {
      const auto expected_id = midismith::protocol_can::CanIdentifierMapper::EncodeId(
          MainBoardMessageBuilder().BuildStopAdc(5).first);

      When(fakeit_Method(transceiver_mock, Transmit)).Do(capture_frame);

      sender.SendStopAdc(5);

      SECTION("Should transmit a frame with the correct CAN identifier") {
        Verify(fakeit_Method(transceiver_mock, Transmit)).AtLeastOnce();
        REQUIRE(captured_frame.identifier == expected_id);
      }

      SECTION("Should transmit a frame with DLC = 1") {
        Verify(fakeit_Method(transceiver_mock, Transmit)).AtLeastOnce();
        REQUIRE(captured_frame.data_length_bytes ==
                midismith::protocol::AdcStop::kSerializedSizeBytes);
      }
    }
  }

  SECTION("The SendStartCalibration() method") {
    SECTION("When called with CalibMode::kAuto") {
      const auto expected_id = midismith::protocol_can::CanIdentifierMapper::EncodeId(
          MainBoardMessageBuilder().BuildStartCalibration(2, CalibMode::kAuto).first);

      When(fakeit_Method(transceiver_mock, Transmit)).Do(capture_frame);

      sender.SendStartCalibration(2, CalibMode::kAuto);

      SECTION("Should transmit a frame with the correct CAN identifier") {
        Verify(fakeit_Method(transceiver_mock, Transmit)).AtLeastOnce();
        REQUIRE(captured_frame.identifier == expected_id);
      }

      SECTION("Should transmit a frame with DLC = 2") {
        Verify(fakeit_Method(transceiver_mock, Transmit)).AtLeastOnce();
        REQUIRE(captured_frame.data_length_bytes ==
                midismith::protocol::CalibStart::kSerializedSizeBytes);
      }

      SECTION("Should encode kAuto in the second payload byte") {
        Verify(fakeit_Method(transceiver_mock, Transmit)).AtLeastOnce();
        REQUIRE(captured_frame.data[1] == static_cast<std::uint8_t>(CalibMode::kAuto));
      }
    }

    SECTION("When called with CalibMode::kManual") {
      When(fakeit_Method(transceiver_mock, Transmit)).Do(capture_frame);

      sender.SendStartCalibration(2, CalibMode::kManual);

      SECTION("Should encode kManual in the second payload byte") {
        Verify(fakeit_Method(transceiver_mock, Transmit)).AtLeastOnce();
        REQUIRE(captured_frame.data[1] == static_cast<std::uint8_t>(CalibMode::kManual));
      }
    }
  }

  SECTION("The SendCalibrationAck() method") {
    const auto expected_id = midismith::protocol_can::CanIdentifierMapper::EncodeId(
        MainBoardMessageBuilder().BuildDataSegmentAck(3, 5, DataSegmentAckStatus::kOk).first);

    When(fakeit_Method(transceiver_mock, Transmit)).Do(capture_frame);

    sender.SendCalibrationAck(3, 5, DataSegmentAckStatus::kOk);

    SECTION("Should transmit a frame with the correct CAN identifier") {
      Verify(fakeit_Method(transceiver_mock, Transmit)).AtLeastOnce();
      REQUIRE(captured_frame.identifier == expected_id);
    }

    SECTION("Should transmit a frame with DLC = 2") {
      Verify(fakeit_Method(transceiver_mock, Transmit)).AtLeastOnce();
      REQUIRE(captured_frame.data_length_bytes ==
              midismith::protocol::DataSegmentAck::kSerializedSizeBytes);
    }
  }
}

#endif

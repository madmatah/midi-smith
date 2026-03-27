#if defined(UNIT_TESTS)

#include "app/messaging/adc_board_can_message_sender.hpp"

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

class RecordingFdcanTransceiverStub final
    : public midismith::bsp::can::FdcanTransceiverRequirements {
 public:
  bool Transmit(const midismith::bsp::can::FdcanFrame& frame) noexcept override {
    ++transmit_call_count;
    last_frame = frame;
    return true;
  }

  std::uint32_t transmit_call_count = 0;
  midismith::bsp::can::FdcanFrame last_frame{};
};

}  // namespace

using midismith::adc_board::app::messaging::AdcBoardCanMessageSender;
using midismith::protocol::AdcMessageBuilder;
using midismith::protocol::DataSegmentAckStatus;
using midismith::protocol::DeviceState;
using midismith::protocol::SensorEventType;

TEST_CASE("The AdcBoardCanMessageSender class") {
  SECTION("The SendNoteOn() method") {
    SECTION("When called with a valid sensor id and velocity") {
      SECTION("Should transmit with the expected CAN identifier and payload") {
        Mock<midismith::bsp::can::FdcanTransceiverRequirements> transceiver_mock;
        std::uint8_t board_id = 2;
        AdcBoardCanMessageSender sender(transceiver_mock.get(), board_id);
        midismith::bsp::can::FdcanFrame captured_frame{};
        const auto expected_id = midismith::protocol_can::CanIdentifierMapper::EncodeId(
            AdcMessageBuilder(board_id).BuildNoteOn(42, 100).first);

        When(fakeit_Method(transceiver_mock, Transmit))
            .AlwaysDo([&](const midismith::bsp::can::FdcanFrame& frame) {
              captured_frame = frame;
              return true;
            });

        sender.SendNoteOn(42, 100);

        Verify(fakeit_Method(transceiver_mock, Transmit)).Once();
        REQUIRE(captured_frame.identifier == expected_id);
        REQUIRE(captured_frame.data_length_bytes == 3);
        REQUIRE(captured_frame.data[0] == static_cast<std::uint8_t>(SensorEventType::kNoteOn));
        REQUIRE(captured_frame.data[1] == 42);
        REQUIRE(captured_frame.data[2] == 100);
      }
    }
  }

  SECTION("The SendNoteOff() method") {
    SECTION("When called with a valid sensor id and velocity") {
      SECTION("Should transmit with the expected CAN identifier and payload") {
        Mock<midismith::bsp::can::FdcanTransceiverRequirements> transceiver_mock;
        std::uint8_t board_id = 2;
        AdcBoardCanMessageSender sender(transceiver_mock.get(), board_id);
        midismith::bsp::can::FdcanFrame captured_frame{};
        const auto expected_id = midismith::protocol_can::CanIdentifierMapper::EncodeId(
            AdcMessageBuilder(board_id).BuildNoteOff(42, 64).first);

        When(fakeit_Method(transceiver_mock, Transmit))
            .AlwaysDo([&](const midismith::bsp::can::FdcanFrame& frame) {
              captured_frame = frame;
              return true;
            });

        sender.SendNoteOff(42, 64);

        Verify(fakeit_Method(transceiver_mock, Transmit)).Once();
        REQUIRE(captured_frame.identifier == expected_id);
        REQUIRE(captured_frame.data_length_bytes == 3);
        REQUIRE(captured_frame.data[0] == static_cast<std::uint8_t>(SensorEventType::kNoteOff));
        REQUIRE(captured_frame.data[1] == 42);
        REQUIRE(captured_frame.data[2] == 64);
      }
    }
  }

  SECTION("The SendHeartbeat() method") {
    SECTION("When called with a running device state") {
      SECTION("Should transmit with the expected CAN identifier and payload") {
        Mock<midismith::bsp::can::FdcanTransceiverRequirements> transceiver_mock;
        std::uint8_t board_id = 2;
        AdcBoardCanMessageSender sender(transceiver_mock.get(), board_id);
        midismith::bsp::can::FdcanFrame captured_frame{};
        const auto expected_id = midismith::protocol_can::CanIdentifierMapper::EncodeId(
            AdcMessageBuilder(board_id).BuildHeartbeat(DeviceState::kRunning).first);

        When(fakeit_Method(transceiver_mock, Transmit))
            .Do([&](const midismith::bsp::can::FdcanFrame& frame) {
              captured_frame = frame;
              return true;
            });

        sender.SendHeartbeat(DeviceState::kRunning);

        Verify(fakeit_Method(transceiver_mock, Transmit)).Once();
        REQUIRE(captured_frame.identifier == expected_id);
        REQUIRE(captured_frame.data_length_bytes == 1);
        REQUIRE(captured_frame.data[0] == static_cast<std::uint8_t>(DeviceState::kRunning));
      }
    }
  }

  SECTION("The SendCalibrationLoadRequest() method") {
    SECTION("When called by an ADC node") {
      SECTION("Should transmit a command frame with action code 0x05") {
        Mock<midismith::bsp::can::FdcanTransceiverRequirements> transceiver_mock;
        std::uint8_t board_id = 2;
        AdcBoardCanMessageSender sender(transceiver_mock.get(), board_id);
        midismith::bsp::can::FdcanFrame captured_frame{};
        const auto expected_id = midismith::protocol_can::CanIdentifierMapper::EncodeId(
            AdcMessageBuilder(board_id).BuildCalibrationLoadRequest().first);

        When(fakeit_Method(transceiver_mock, Transmit))
            .Do([&](const midismith::bsp::can::FdcanFrame& frame) {
              captured_frame = frame;
              return true;
            });

        sender.SendCalibrationLoadRequest();

        Verify(fakeit_Method(transceiver_mock, Transmit)).Once();
        REQUIRE(captured_frame.identifier == expected_id);
        REQUIRE(captured_frame.data_length_bytes == 1);
        REQUIRE(
            captured_frame.data[0] ==
            static_cast<std::uint8_t>(midismith::protocol::CommandAction::kCalibrationLoadRequest));
      }
    }
  }

  SECTION("The SendDataSegmentAck() method") {
    SECTION("When called with an ack index and status") {
      SECTION("Should transmit with the expected CAN identifier and ack payload") {
        Mock<midismith::bsp::can::FdcanTransceiverRequirements> transceiver_mock;
        std::uint8_t board_id = 4;
        AdcBoardCanMessageSender sender(transceiver_mock.get(), board_id);
        midismith::bsp::can::FdcanFrame captured_frame{};
        const auto expected_id = midismith::protocol_can::CanIdentifierMapper::EncodeId(
            AdcMessageBuilder(board_id)
                .BuildDataSegmentAck(6, DataSegmentAckStatus::kCrcError)
                .first);

        When(fakeit_Method(transceiver_mock, Transmit))
            .Do([&](const midismith::bsp::can::FdcanFrame& frame) {
              captured_frame = frame;
              return true;
            });

        sender.SendDataSegmentAck(6, DataSegmentAckStatus::kCrcError);

        Verify(fakeit_Method(transceiver_mock, Transmit)).Once();
        REQUIRE(captured_frame.identifier == expected_id);
        REQUIRE(captured_frame.data_length_bytes == 2);
        REQUIRE(captured_frame.data[0] == 6);
        REQUIRE(captured_frame.data[1] ==
                static_cast<std::uint8_t>(DataSegmentAckStatus::kCrcError));
      }
    }
  }

  SECTION("The dynamic board_id behavior") {
    SECTION("When the referenced board_id changes after construction") {
      SECTION("Should use the updated board_id in subsequent CAN identifiers") {
        RecordingFdcanTransceiverStub transceiver_stub;
        std::uint8_t board_id = 1;
        AdcBoardCanMessageSender sender(transceiver_stub, board_id);

        sender.SendNoteOn(5, 80);
        board_id = 5;
        sender.SendNoteOn(5, 80);

        const auto expected_identifier_after_change =
            midismith::protocol_can::CanIdentifierMapper::EncodeId(
                AdcMessageBuilder(5).BuildNoteOn(5, 80).first);

        REQUIRE(transceiver_stub.transmit_call_count == 2);
        REQUIRE(transceiver_stub.last_frame.identifier == expected_identifier_after_change);
      }
    }
  }
}

#endif

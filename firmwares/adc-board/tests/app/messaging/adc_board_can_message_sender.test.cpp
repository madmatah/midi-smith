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

}  // namespace

using midismith::adc_board::app::messaging::AdcBoardCanMessageSender;
using midismith::protocol::AdcMessageBuilder;
using midismith::protocol::DeviceState;
using midismith::protocol::SensorEventType;

TEST_CASE("AdcBoardCanMessageSender — SendNoteOn") {
  Mock<midismith::bsp::can::FdcanTransceiverRequirements> transceiver_mock;
  std::uint8_t board_id = 2;
  AdcBoardCanMessageSender sender(transceiver_mock.get(), board_id);
  midismith::bsp::can::FdcanFrame captured_frame{};

  auto capture_frame = [&](const midismith::bsp::can::FdcanFrame& f) {
    captured_frame = f;
    return true;
  };

  SECTION("Should transmit a frame with the correct CAN identifier") {
    const auto expected_id = midismith::protocol_can::CanIdentifierMapper::EncodeId(
        AdcMessageBuilder(board_id).BuildNoteOn(10, 80).first);

    When(fakeit_Method(transceiver_mock, Transmit)).Do(capture_frame);

    sender.SendNoteOn(10, 80);

    Verify(fakeit_Method(transceiver_mock, Transmit)).Once();
    REQUIRE(captured_frame.identifier == expected_id);
  }

  SECTION("Should transmit a frame with DLC = 3") {
    When(fakeit_Method(transceiver_mock, Transmit)).Do(capture_frame);

    sender.SendNoteOn(10, 80);

    Verify(fakeit_Method(transceiver_mock, Transmit)).Once();
    REQUIRE(captured_frame.data_length_bytes == 3);
  }

  SECTION("Should encode the payload as type, sensor_id, velocity") {
    When(fakeit_Method(transceiver_mock, Transmit)).Do(capture_frame);

    sender.SendNoteOn(42, 100);

    Verify(fakeit_Method(transceiver_mock, Transmit)).Once();
    REQUIRE(captured_frame.data[0] == static_cast<std::uint8_t>(SensorEventType::kNoteOn));
    REQUIRE(captured_frame.data[1] == 42);
    REQUIRE(captured_frame.data[2] == 100);
  }
}

TEST_CASE("AdcBoardCanMessageSender — SendNoteOff") {
  Mock<midismith::bsp::can::FdcanTransceiverRequirements> transceiver_mock;
  std::uint8_t board_id = 2;
  AdcBoardCanMessageSender sender(transceiver_mock.get(), board_id);
  midismith::bsp::can::FdcanFrame captured_frame{};

  auto capture_frame = [&](const midismith::bsp::can::FdcanFrame& f) {
    captured_frame = f;
    return true;
  };

  SECTION("Should transmit a frame with the correct CAN identifier") {
    const auto expected_id = midismith::protocol_can::CanIdentifierMapper::EncodeId(
        AdcMessageBuilder(board_id).BuildNoteOff(10, 64).first);

    When(fakeit_Method(transceiver_mock, Transmit)).Do(capture_frame);

    sender.SendNoteOff(10, 64);

    Verify(fakeit_Method(transceiver_mock, Transmit)).Once();
    REQUIRE(captured_frame.identifier == expected_id);
  }

  SECTION("Should transmit a frame with DLC = 3") {
    When(fakeit_Method(transceiver_mock, Transmit)).Do(capture_frame);

    sender.SendNoteOff(10, 64);

    Verify(fakeit_Method(transceiver_mock, Transmit)).Once();
    REQUIRE(captured_frame.data_length_bytes == 3);
  }

  SECTION("Should encode the payload as type, sensor_id, velocity") {
    When(fakeit_Method(transceiver_mock, Transmit)).Do(capture_frame);

    sender.SendNoteOff(42, 64);

    Verify(fakeit_Method(transceiver_mock, Transmit)).AtLeastOnce();
    REQUIRE(captured_frame.data[0] == static_cast<std::uint8_t>(SensorEventType::kNoteOff));
    REQUIRE(captured_frame.data[1] == 42);
    REQUIRE(captured_frame.data[2] == 64);
  }
}

TEST_CASE("AdcBoardCanMessageSender — SendHeartbeat") {
  Mock<midismith::bsp::can::FdcanTransceiverRequirements> transceiver_mock;
  std::uint8_t board_id = 2;
  AdcBoardCanMessageSender sender(transceiver_mock.get(), board_id);
  midismith::bsp::can::FdcanFrame captured_frame{};

  auto capture_frame = [&](const midismith::bsp::can::FdcanFrame& f) {
    captured_frame = f;
    return true;
  };

  SECTION("Should transmit a frame with the correct CAN identifier") {
    const auto expected_id = midismith::protocol_can::CanIdentifierMapper::EncodeId(
        AdcMessageBuilder(board_id).BuildHeartbeat(DeviceState::kRunning).first);

    When(fakeit_Method(transceiver_mock, Transmit)).Do(capture_frame);

    sender.SendHeartbeat(DeviceState::kRunning);

    Verify(fakeit_Method(transceiver_mock, Transmit)).Once();
    REQUIRE(captured_frame.identifier == expected_id);
  }

  SECTION("Should transmit a frame with DLC = 1") {
    When(fakeit_Method(transceiver_mock, Transmit)).Do(capture_frame);

    sender.SendHeartbeat(DeviceState::kRunning);

    Verify(fakeit_Method(transceiver_mock, Transmit)).Once();
    REQUIRE(captured_frame.data_length_bytes == 1);
  }

  SECTION("Should encode the device state as the single payload byte") {
    When(fakeit_Method(transceiver_mock, Transmit)).Do(capture_frame);

    sender.SendHeartbeat(DeviceState::kRunning);

    Verify(fakeit_Method(transceiver_mock, Transmit)).Once();
    REQUIRE(captured_frame.data[0] == static_cast<std::uint8_t>(DeviceState::kRunning));
  }
}

TEST_CASE("AdcBoardCanMessageSender — dynamic board_id") {
  Mock<midismith::bsp::can::FdcanTransceiverRequirements> transceiver_mock;
  std::uint8_t board_id = 1;
  AdcBoardCanMessageSender sender(transceiver_mock.get(), board_id);
  midismith::bsp::can::FdcanFrame captured_frame{};

  auto capture_frame = [&](const midismith::bsp::can::FdcanFrame& f) {
    captured_frame = f;
    return true;
  };

  SECTION("Should reflect the updated board_id in the CAN identifier after a change") {
    When(fakeit_Method(transceiver_mock, Transmit)).AlwaysDo(capture_frame);

    sender.SendNoteOn(5, 80);

    board_id = 5;
    sender.SendNoteOn(5, 80);

    const auto id_after = midismith::protocol_can::CanIdentifierMapper::EncodeId(
        AdcMessageBuilder(5).BuildNoteOn(5, 80).first);

    // Verify it was called twice with different identifiers
    Verify(fakeit_Method(transceiver_mock, Transmit)).Exactly(2);
    REQUIRE(captured_frame.identifier == id_after);
  }
}

#endif

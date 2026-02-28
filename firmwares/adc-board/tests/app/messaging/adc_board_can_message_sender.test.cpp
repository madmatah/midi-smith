#if defined(UNIT_TESTS)

#include "app/messaging/adc_board_can_message_sender.hpp"

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

using midismith::adc_board::app::messaging::AdcBoardCanMessageSender;
using midismith::protocol::AdcMessageBuilder;
using midismith::protocol::DeviceState;
using midismith::protocol::SensorEventType;

TEST_CASE("AdcBoardCanMessageSender — SendNoteOn") {
  RecordingTransceiver transceiver;
  std::uint8_t board_id = 2;
  AdcBoardCanMessageSender sender(transceiver, board_id);

  SECTION("Should transmit a frame with the correct CAN identifier") {
    sender.SendNoteOn(10, 80);

    const auto expected_id = midismith::protocol_can::CanIdentifierMapper::EncodeId(
        AdcMessageBuilder(board_id).BuildNoteOn(10, 80).first);
    REQUIRE(transceiver.last_frame().has_value());
    REQUIRE(transceiver.last_frame()->identifier == expected_id);
  }

  SECTION("Should transmit a frame with DLC = 3") {
    sender.SendNoteOn(10, 80);

    REQUIRE(transceiver.last_frame().has_value());
    REQUIRE(transceiver.last_frame()->data_length_bytes == 3);
  }

  SECTION("Should encode the payload as type, sensor_id, velocity") {
    sender.SendNoteOn(42, 100);

    REQUIRE(transceiver.last_frame().has_value());
    REQUIRE(transceiver.last_frame()->data[0] ==
            static_cast<std::uint8_t>(SensorEventType::kNoteOn));
    REQUIRE(transceiver.last_frame()->data[1] == 42);
    REQUIRE(transceiver.last_frame()->data[2] == 100);
  }
}

TEST_CASE("AdcBoardCanMessageSender — SendNoteOff") {
  RecordingTransceiver transceiver;
  std::uint8_t board_id = 2;
  AdcBoardCanMessageSender sender(transceiver, board_id);

  SECTION("Should transmit a frame with the correct CAN identifier") {
    sender.SendNoteOff(10, 64);

    const auto expected_id = midismith::protocol_can::CanIdentifierMapper::EncodeId(
        AdcMessageBuilder(board_id).BuildNoteOff(10, 64).first);
    REQUIRE(transceiver.last_frame().has_value());
    REQUIRE(transceiver.last_frame()->identifier == expected_id);
  }

  SECTION("Should transmit a frame with DLC = 3") {
    sender.SendNoteOff(10, 64);

    REQUIRE(transceiver.last_frame().has_value());
    REQUIRE(transceiver.last_frame()->data_length_bytes == 3);
  }

  SECTION("Should encode the payload as type, sensor_id, velocity") {
    sender.SendNoteOff(42, 64);

    REQUIRE(transceiver.last_frame().has_value());
    REQUIRE(transceiver.last_frame()->data[0] ==
            static_cast<std::uint8_t>(SensorEventType::kNoteOff));
    REQUIRE(transceiver.last_frame()->data[1] == 42);
    REQUIRE(transceiver.last_frame()->data[2] == 64);
  }
}

TEST_CASE("AdcBoardCanMessageSender — SendHeartbeat") {
  RecordingTransceiver transceiver;
  std::uint8_t board_id = 2;
  AdcBoardCanMessageSender sender(transceiver, board_id);

  SECTION("Should transmit a frame with the correct CAN identifier") {
    sender.SendHeartbeat(DeviceState::kRunning);

    const auto expected_id = midismith::protocol_can::CanIdentifierMapper::EncodeId(
        AdcMessageBuilder(board_id).BuildHeartbeat(DeviceState::kRunning).first);
    REQUIRE(transceiver.last_frame().has_value());
    REQUIRE(transceiver.last_frame()->identifier == expected_id);
  }

  SECTION("Should transmit a frame with DLC = 1") {
    sender.SendHeartbeat(DeviceState::kRunning);

    REQUIRE(transceiver.last_frame().has_value());
    REQUIRE(transceiver.last_frame()->data_length_bytes == 1);
  }

  SECTION("Should encode the device state as the single payload byte") {
    sender.SendHeartbeat(DeviceState::kCalibrating);

    REQUIRE(transceiver.last_frame().has_value());
    REQUIRE(transceiver.last_frame()->data[0] ==
            static_cast<std::uint8_t>(DeviceState::kCalibrating));
  }
}

TEST_CASE("AdcBoardCanMessageSender — dynamic board_id") {
  RecordingTransceiver transceiver;
  std::uint8_t board_id = 1;
  AdcBoardCanMessageSender sender(transceiver, board_id);

  SECTION("Should reflect the updated board_id in the CAN identifier after a change") {
    sender.SendNoteOn(5, 80);
    const auto id_before = transceiver.last_frame()->identifier;

    board_id = 5;
    sender.SendNoteOn(5, 80);
    const auto id_after = transceiver.last_frame()->identifier;

    REQUIRE(id_before != id_after);
    REQUIRE(id_after == midismith::protocol_can::CanIdentifierMapper::EncodeId(
                            AdcMessageBuilder(5).BuildNoteOn(5, 80).first));
  }
}

#endif

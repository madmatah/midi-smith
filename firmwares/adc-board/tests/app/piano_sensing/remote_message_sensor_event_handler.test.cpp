#if defined(UNIT_TESTS)

#include "app/piano_sensing/remote_message_sensor_event_handler.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cstdint>

#include "protocol/messages.hpp"

namespace {

class RecordingMessageSender final
    : public midismith::adc_board::app::messaging::AdcBoardMessageSenderRequirements {
 public:
  bool SendNoteOn(std::uint8_t sensor_id, std::uint8_t velocity) noexcept override {
    ++send_note_on_calls;
    last_note_on_sensor_id = sensor_id;
    last_note_on_velocity = velocity;
    return send_note_on_result;
  }

  bool SendNoteOff(std::uint8_t sensor_id, std::uint8_t velocity) noexcept override {
    ++send_note_off_calls;
    last_note_off_sensor_id = sensor_id;
    last_note_off_velocity = velocity;
    return send_note_off_result;
  }

  bool SendHeartbeat(midismith::protocol::DeviceState) noexcept override {
    ++send_heartbeat_calls;
    return true;
  }

  int send_note_on_calls = 0;
  int send_note_off_calls = 0;
  int send_heartbeat_calls = 0;
  std::uint8_t last_note_on_sensor_id = 0u;
  std::uint8_t last_note_off_sensor_id = 0u;
  std::uint8_t last_note_on_velocity = 0u;
  std::uint8_t last_note_off_velocity = 0u;
  bool send_note_on_result = true;
  bool send_note_off_result = true;
};

}  // namespace

TEST_CASE("The RemoteMessageSensorEventHandler class") {
  SECTION("The OnNoteOn method") {
    SECTION("When called with a MIDI velocity") {
      SECTION("Should forward sensor id and velocity to SendNoteOn") {
        RecordingMessageSender message_sender;
        midismith::adc_board::app::piano_sensing::RemoteMessageSensorEventHandler handler(
            message_sender, 9u);

        handler.OnNoteOn(static_cast<midismith::midi::Velocity>(101u));

        REQUIRE(message_sender.send_note_on_calls == 1);
        REQUIRE(message_sender.last_note_on_sensor_id == 9u);
        REQUIRE(message_sender.last_note_on_velocity == 101u);
      }
    }
  }

  SECTION("The OnNoteOff method") {
    SECTION("When called with a MIDI velocity") {
      SECTION("Should forward sensor id and velocity to SendNoteOff") {
        RecordingMessageSender message_sender;
        midismith::adc_board::app::piano_sensing::RemoteMessageSensorEventHandler handler(
            message_sender, 3u);

        handler.OnNoteOff(static_cast<midismith::midi::Velocity>(47u));

        REQUIRE(message_sender.send_note_off_calls == 1);
        REQUIRE(message_sender.last_note_off_sensor_id == 3u);
        REQUIRE(message_sender.last_note_off_velocity == 47u);
      }
    }
  }

  SECTION("The send failure handling") {
    SECTION("When SendNoteOn and SendNoteOff return false") {
      SECTION("Should keep best-effort behavior and still perform both calls") {
        RecordingMessageSender message_sender;
        message_sender.send_note_on_result = false;
        message_sender.send_note_off_result = false;
        midismith::adc_board::app::piano_sensing::RemoteMessageSensorEventHandler handler(
            message_sender, 17u);

        handler.OnNoteOn(static_cast<midismith::midi::Velocity>(63u));
        handler.OnNoteOff(static_cast<midismith::midi::Velocity>(21u));

        REQUIRE(message_sender.send_note_on_calls == 1);
        REQUIRE(message_sender.send_note_off_calls == 1);
        REQUIRE(message_sender.last_note_on_sensor_id == 17u);
        REQUIRE(message_sender.last_note_off_sensor_id == 17u);
      }
    }
  }
}

#endif

#if defined(UNIT_TESTS)

#include "app/messaging/main_board_inbound_sensor_event_midi_handler.hpp"

#include <catch2/catch_test_macros.hpp>
#include <fakeit.hpp>

using fakeit::Mock;
using fakeit::Verify;
using fakeit::When;

#define fakeit_Method(mock, method) Method(mock, method)

TEST_CASE("The MainBoardInboundSensorEventMidiHandler class", "[main-board][app][messaging]") {
  Mock<midismith::piano_controller::PianoRequirements> piano_mock;
  midismith::main_board::app::messaging::MainBoardInboundSensorEventMidiHandler handler(
      piano_mock.get());

  SECTION("When the sensor event is kNoteOn") {
    const midismith::protocol::SensorEvent event = {
        .type = midismith::protocol::SensorEventType::kNoteOn,
        .sensor_id = 9u,
        .velocity = 101u,
    };

    When(fakeit_Method(piano_mock, PressKey))
        .Do([](midismith::midi::NoteNumber note, midismith::midi::Velocity velocity) {
          REQUIRE(note == 64u);
          REQUIRE(velocity == 101u);
        });

    handler.OnSensorEvent(event);

    Verify(fakeit_Method(piano_mock, PressKey)).Once();
    Verify(fakeit_Method(piano_mock, ReleaseKey)).Never();
  }

  SECTION("When the sensor event is kNoteOff") {
    const midismith::protocol::SensorEvent event = {
        .type = midismith::protocol::SensorEventType::kNoteOff,
        .sensor_id = 4u,
        .velocity = 37u,
    };

    When(fakeit_Method(piano_mock, ReleaseKey))
        .Do([](midismith::midi::NoteNumber note, midismith::midi::Velocity velocity) {
          REQUIRE(note == 64u);
          REQUIRE(velocity == 37u);
        });

    handler.OnSensorEvent(event);

    Verify(fakeit_Method(piano_mock, ReleaseKey)).Once();
    Verify(fakeit_Method(piano_mock, PressKey)).Never();
  }
}

#endif

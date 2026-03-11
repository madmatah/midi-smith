#if defined(UNIT_TESTS)

#include "app/messaging/main_board_inbound_sensor_event_midi_handler.hpp"

#include <catch2/catch_test_macros.hpp>
#include <fakeit.hpp>

using fakeit::Mock;
using fakeit::Verify;
using fakeit::When;

#define fakeit_Method(mock, method) Method(mock, method)

namespace {

class StubKeymapLookup final
    : public midismith::main_board::domain::config::KeymapLookupRequirements {
 public:
  std::optional<std::uint8_t> FindMidiNote(std::uint8_t board_id,
                                           std::uint8_t sensor_id) const noexcept override {
    if (board_id == mapped_board_id && sensor_id == mapped_sensor_id) {
      return mapped_midi_note;
    }
    return std::nullopt;
  }

  std::uint8_t mapped_board_id = 1;
  std::uint8_t mapped_sensor_id = 9;
  std::uint8_t mapped_midi_note = 60;
};

}  // namespace

TEST_CASE("The MainBoardInboundSensorEventMidiHandler class", "[main-board][app][messaging]") {
  Mock<midismith::piano_controller::PianoRequirements> piano_mock;
  StubKeymapLookup keymap;
  midismith::main_board::app::messaging::MainBoardInboundSensorEventMidiHandler handler(
      piano_mock.get(), keymap);

  SECTION("When the sensor event has a known keymap mapping") {
    SECTION("When the sensor event is kNoteOn") {
      const midismith::protocol::SensorEvent event = {
          .type = midismith::protocol::SensorEventType::kNoteOn,
          .sensor_id = 9u,
          .velocity = 101u,
      };

      When(fakeit_Method(piano_mock, PressKey))
          .Do([](midismith::midi::NoteNumber note, midismith::midi::Velocity velocity) {
            REQUIRE(note == 60u);
            REQUIRE(velocity == 101u);
          });

      handler.OnSensorEvent(event, 1u);

      Verify(fakeit_Method(piano_mock, PressKey)).Once();
      Verify(fakeit_Method(piano_mock, ReleaseKey)).Never();
    }

    SECTION("When the sensor event is kNoteOff") {
      const midismith::protocol::SensorEvent event = {
          .type = midismith::protocol::SensorEventType::kNoteOff,
          .sensor_id = 9u,
          .velocity = 37u,
      };

      When(fakeit_Method(piano_mock, ReleaseKey))
          .Do([](midismith::midi::NoteNumber note, midismith::midi::Velocity velocity) {
            REQUIRE(note == 60u);
            REQUIRE(velocity == 37u);
          });

      handler.OnSensorEvent(event, 1u);

      Verify(fakeit_Method(piano_mock, ReleaseKey)).Once();
      Verify(fakeit_Method(piano_mock, PressKey)).Never();
    }
  }

  SECTION("When the sensor event has no keymap mapping") {
    const midismith::protocol::SensorEvent event = {
        .type = midismith::protocol::SensorEventType::kNoteOn,
        .sensor_id = 0u,
        .velocity = 100u,
    };

    handler.OnSensorEvent(event, 5u);

    Verify(fakeit_Method(piano_mock, PressKey)).Never();
    Verify(fakeit_Method(piano_mock, ReleaseKey)).Never();
  }
}

#endif

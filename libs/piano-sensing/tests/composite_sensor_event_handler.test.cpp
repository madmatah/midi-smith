#if defined(UNIT_TESTS)

#include "piano-sensing/composite_sensor_event_handler.hpp"

#include <array>
#include <catch2/catch_test_macros.hpp>
#include <functional>

namespace {

class RecordingKeyActionHandler final : public midismith::piano_sensing::KeyActionRequirements {
 public:
  void OnNoteOn(midismith::midi::Velocity velocity) noexcept override {
    ++note_on_calls;
    last_note_on_velocity = velocity;
  }

  void OnNoteOff(midismith::midi::Velocity release_velocity) noexcept override {
    ++note_off_calls;
    last_note_off_velocity = release_velocity;
  }

  int note_on_calls = 0;
  int note_off_calls = 0;
  midismith::midi::Velocity last_note_on_velocity = 0u;
  midismith::midi::Velocity last_note_off_velocity = 0u;
};

}  // namespace

TEST_CASE("The CompositeSensorEventHandler class") {
  SECTION("The OnNoteOn method") {
    SECTION("When two handlers are registered") {
      SECTION("Should dispatch the event to both handlers") {
        RecordingKeyActionHandler first_handler;
        RecordingKeyActionHandler second_handler;
        midismith::piano_sensing::CompositeSensorEventHandler<2> composite(
            std::array<std::reference_wrapper<midismith::piano_sensing::KeyActionRequirements>, 2>{
                std::ref(
                    static_cast<midismith::piano_sensing::KeyActionRequirements&>(first_handler)),
                std::ref(static_cast<midismith::piano_sensing::KeyActionRequirements&>(
                    second_handler))});

        composite.OnNoteOn(static_cast<midismith::midi::Velocity>(91u));

        REQUIRE(first_handler.note_on_calls == 1);
        REQUIRE(second_handler.note_on_calls == 1);
        REQUIRE(first_handler.last_note_on_velocity == static_cast<midismith::midi::Velocity>(91u));
        REQUIRE(second_handler.last_note_on_velocity ==
                static_cast<midismith::midi::Velocity>(91u));
      }
    }
  }

  SECTION("The OnNoteOff method") {
    SECTION("When two handlers are registered") {
      SECTION("Should dispatch the event to both handlers") {
        RecordingKeyActionHandler first_handler;
        RecordingKeyActionHandler second_handler;
        midismith::piano_sensing::CompositeSensorEventHandler<2> composite(
            std::array<std::reference_wrapper<midismith::piano_sensing::KeyActionRequirements>, 2>{
                std::ref(
                    static_cast<midismith::piano_sensing::KeyActionRequirements&>(first_handler)),
                std::ref(static_cast<midismith::piano_sensing::KeyActionRequirements&>(
                    second_handler))});

        composite.OnNoteOff(static_cast<midismith::midi::Velocity>(44u));

        REQUIRE(first_handler.note_off_calls == 1);
        REQUIRE(second_handler.note_off_calls == 1);
        REQUIRE(first_handler.last_note_off_velocity ==
                static_cast<midismith::midi::Velocity>(44u));
        REQUIRE(second_handler.last_note_off_velocity ==
                static_cast<midismith::midi::Velocity>(44u));
      }
    }
  }

  SECTION("The zero-handler specialization") {
    SECTION("When no handlers are provided") {
      SECTION("Should accept note events without side effects") {
        midismith::piano_sensing::CompositeSensorEventHandler<0> composite(
            std::array<std::reference_wrapper<midismith::piano_sensing::KeyActionRequirements>,
                       0>{});

        composite.OnNoteOn(static_cast<midismith::midi::Velocity>(10u));
        composite.OnNoteOff(static_cast<midismith::midi::Velocity>(20u));

        REQUIRE(true);
      }
    }
  }
}

#endif

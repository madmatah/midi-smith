#if defined(UNIT_TESTS)

#include "piano-sensing/composite_sensor_event_handler.hpp"

#include <fakeit.hpp>

#include <array>
#include <catch2/catch_test_macros.hpp>
#include <functional>

namespace {

using fakeit::Mock;
using fakeit::Verify;
using fakeit::When;
using fakeit::Fake;

#define fakeit_Method(mock, method) Method(mock, method)

}  // namespace

TEST_CASE("The CompositeSensorEventHandler class") {
  SECTION("The OnNoteOn method") {
    SECTION("When two handlers are registered") {
      SECTION("Should dispatch the event to both handlers") {
        Mock<midismith::piano_sensing::KeyActionRequirements> first_mock;
        Mock<midismith::piano_sensing::KeyActionRequirements> second_mock;

        Fake(fakeit_Method(first_mock, OnNoteOn));
        Fake(fakeit_Method(second_mock, OnNoteOn));

        midismith::piano_sensing::CompositeSensorEventHandler<2> composite(
            std::array<std::reference_wrapper<midismith::piano_sensing::KeyActionRequirements>, 2>{
                std::ref(first_mock.get()), std::ref(second_mock.get())});

        const auto velocity = static_cast<midismith::midi::Velocity>(91u);
        composite.OnNoteOn(velocity);

        Verify(fakeit_Method(first_mock, OnNoteOn).Using(velocity)).Once();
        Verify(fakeit_Method(second_mock, OnNoteOn).Using(velocity)).Once();
      }
    }
  }

  SECTION("The OnNoteOff method") {
    SECTION("When two handlers are registered") {
      SECTION("Should dispatch the event to both handlers") {
        Mock<midismith::piano_sensing::KeyActionRequirements> first_mock;
        Mock<midismith::piano_sensing::KeyActionRequirements> second_mock;

        Fake(fakeit_Method(first_mock, OnNoteOff));
        Fake(fakeit_Method(second_mock, OnNoteOff));

        midismith::piano_sensing::CompositeSensorEventHandler<2> composite(
            std::array<std::reference_wrapper<midismith::piano_sensing::KeyActionRequirements>, 2>{
                std::ref(first_mock.get()), std::ref(second_mock.get())});

        const auto velocity = static_cast<midismith::midi::Velocity>(44u);
        composite.OnNoteOff(velocity);

        Verify(fakeit_Method(first_mock, OnNoteOff).Using(velocity)).Once();
        Verify(fakeit_Method(second_mock, OnNoteOff).Using(velocity)).Once();
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

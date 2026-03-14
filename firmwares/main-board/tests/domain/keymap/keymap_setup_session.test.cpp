#if defined(UNIT_TESTS)

#include "domain/keymap/keymap_setup_session.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("The KeymapSetupSession class", "[main-board][domain][keymap]") {
  midismith::main_board::domain::keymap::KeymapSetupSession session;

  SECTION("Initial state") {
    REQUIRE_FALSE(session.is_in_progress());
    REQUIRE(session.key_count() == 0u);
    REQUIRE(session.start_note() == 0u);
    REQUIRE(session.captured_count() == 0u);
  }

  SECTION("After Start") {
    session.Start(88u, 21u);

    REQUIRE(session.is_in_progress());
    REQUIRE(session.key_count() == 88u);
    REQUIRE(session.start_note() == 21u);
    REQUIRE(session.captured_count() == 0u);
    REQUIRE(session.next_midi_note() == 21u);
  }

  SECTION("AdvanceCapture when not in progress returns kNotInProgress") {
    auto result = session.AdvanceCapture();
    REQUIRE(result == midismith::main_board::domain::keymap::CaptureResult::kNotInProgress);
  }

  SECTION("AdvanceCapture when in progress increments captured count and returns kCaptured") {
    session.Start(3u, 60u);

    auto result = session.AdvanceCapture();

    REQUIRE(result == midismith::main_board::domain::keymap::CaptureResult::kCaptured);
    REQUIRE(session.captured_count() == 1u);
    REQUIRE(session.next_midi_note() == 61u);
    REQUIRE(session.is_in_progress());
  }

  SECTION("AdvanceCapture returns kComplete when the last key is captured") {
    session.Start(3u, 60u);
    session.AdvanceCapture();
    session.AdvanceCapture();

    auto result = session.AdvanceCapture();

    REQUIRE(result == midismith::main_board::domain::keymap::CaptureResult::kComplete);
    REQUIRE(session.captured_count() == 3u);
    REQUIRE_FALSE(session.is_in_progress());
  }

  SECTION("AdvanceCapture returns kNotInProgress after session completes") {
    session.Start(1u, 60u);
    session.AdvanceCapture();

    auto result = session.AdvanceCapture();

    REQUIRE(result == midismith::main_board::domain::keymap::CaptureResult::kNotInProgress);
  }

  SECTION("Reset clears all state") {
    session.Start(88u, 21u);
    session.AdvanceCapture();
    session.AdvanceCapture();

    session.Reset();

    REQUIRE_FALSE(session.is_in_progress());
    REQUIRE(session.captured_count() == 0u);
  }

  SECTION("Start with key_count of 1 completes on the first AdvanceCapture") {
    session.Start(1u, 0u);

    REQUIRE(session.next_midi_note() == 0u);

    auto result = session.AdvanceCapture();

    REQUIRE(result == midismith::main_board::domain::keymap::CaptureResult::kComplete);
    REQUIRE_FALSE(session.is_in_progress());
  }

  SECTION("Session can be restarted after completion") {
    session.Start(1u, 60u);
    session.AdvanceCapture();
    REQUIRE_FALSE(session.is_in_progress());

    session.Start(2u, 48u);

    REQUIRE(session.is_in_progress());
    REQUIRE(session.key_count() == 2u);
    REQUIRE(session.start_note() == 48u);
    REQUIRE(session.captured_count() == 0u);
  }

  SECTION("Session can be restarted after Reset") {
    session.Start(88u, 21u);
    session.Reset();

    session.Start(5u, 36u);

    REQUIRE(session.is_in_progress());
    REQUIRE(session.key_count() == 5u);
    REQUIRE(session.start_note() == 36u);
  }
}

#endif

#if defined(UNIT_TESTS)

#include "app/ui/idle_tracker.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("The IdleTracker class") {
  SECTION("The Tick() method") {
    SECTION("When the timeout is reached") {
      SECTION("Should report the idle transition exactly once") {
        midismith::main_board::app::ui::IdleTracker tracker(3);

        REQUIRE(!tracker.Tick());
        REQUIRE(!tracker.Tick());
        REQUIRE(tracker.Tick());

        REQUIRE(tracker.is_idle());
        REQUIRE(!tracker.Tick());
      }
    }

    SECTION("When activity is noted before the timeout") {
      SECTION("Should restart the countdown") {
        midismith::main_board::app::ui::IdleTracker tracker(2);

        REQUIRE(!tracker.Tick());
        tracker.NoteActivity();

        REQUIRE(!tracker.Tick());
        REQUIRE(tracker.Tick());
      }
    }
  }

  SECTION("The NoteActivity() method") {
    SECTION("When the tracker is already idle") {
      SECTION("Should leave the idle state") {
        midismith::main_board::app::ui::IdleTracker tracker(1);
        static_cast<void>(tracker.Tick());
        REQUIRE(tracker.is_idle());

        tracker.NoteActivity();

        REQUIRE(!tracker.is_idle());
      }
    }
  }
}

#endif

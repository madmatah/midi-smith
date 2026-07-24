#if defined(UNIT_TESTS)

#include "splash/easing.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using Catch::Matchers::WithinAbs;

TEST_CASE("The easing functions") {
  SECTION("The Clamp() function") {
    SECTION("When the value is inside the range") {
      SECTION("Should return the value unchanged") {
        REQUIRE_THAT(midismith::splash::Clamp(0.4, 0.0, 1.0), WithinAbs(0.4, 1e-12));
      }
    }
    SECTION("When the value is outside the range") {
      SECTION("Should return the nearest bound") {
        REQUIRE_THAT(midismith::splash::Clamp(-2.0, 0.0, 1.0), WithinAbs(0.0, 1e-12));
        REQUIRE_THAT(midismith::splash::Clamp(3.0, 0.0, 1.0), WithinAbs(1.0, 1e-12));
      }
    }
  }

  SECTION("The SmoothStep() function") {
    SECTION("When the progress reaches the bounds") {
      SECTION("Should saturate at 0 and 1") {
        REQUIRE_THAT(midismith::splash::SmoothStep(-0.5), WithinAbs(0.0, 1e-12));
        REQUIRE_THAT(midismith::splash::SmoothStep(0.0), WithinAbs(0.0, 1e-12));
        REQUIRE_THAT(midismith::splash::SmoothStep(1.0), WithinAbs(1.0, 1e-12));
        REQUIRE_THAT(midismith::splash::SmoothStep(1.5), WithinAbs(1.0, 1e-12));
      }
    }
    SECTION("When the progress is midway") {
      SECTION("Should ease through the midpoint") {
        REQUIRE_THAT(midismith::splash::SmoothStep(0.5), WithinAbs(0.5, 1e-12));
      }
    }
  }

  SECTION("The EaseInCubic() function") {
    SECTION("When the progress is midway") {
      SECTION("Should return the cubic of the progress") {
        REQUIRE_THAT(midismith::splash::EaseInCubic(0.5), WithinAbs(0.125, 1e-12));
      }
    }
  }

  SECTION("The EaseOutCubic() function") {
    SECTION("When the progress is midway") {
      SECTION("Should return the mirrored cubic of the progress") {
        REQUIRE_THAT(midismith::splash::EaseOutCubic(0.5), WithinAbs(0.875, 1e-12));
      }
    }
  }

  SECTION("The PhaseProgress() function") {
    SECTION("When the time is inside the phase") {
      SECTION("Should return the normalized progress") {
        REQUIRE_THAT(midismith::splash::PhaseProgress(1.5, 1.0, 2.0), WithinAbs(0.5, 1e-12));
      }
    }
    SECTION("When the time is outside the phase") {
      SECTION("Should saturate at 0 and 1") {
        REQUIRE_THAT(midismith::splash::PhaseProgress(0.5, 1.0, 2.0), WithinAbs(0.0, 1e-12));
        REQUIRE_THAT(midismith::splash::PhaseProgress(2.5, 1.0, 2.0), WithinAbs(1.0, 1e-12));
      }
    }
    SECTION("When the phase is degenerate") {
      SECTION("Should return a completed progress") {
        REQUIRE_THAT(midismith::splash::PhaseProgress(0.0, 2.0, 2.0), WithinAbs(1.0, 1e-12));
      }
    }
  }
}

#endif

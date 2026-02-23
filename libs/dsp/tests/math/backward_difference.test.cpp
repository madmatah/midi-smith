#if defined(UNIT_TESTS)

#include "dsp/math/backward_difference.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cstdint>

using Catch::Matchers::WithinAbs;

namespace {
struct MockContext {
  std::uint32_t timestamp_ticks = 0;
};
}  // namespace

TEST_CASE("The BackwardDifference class") {
  midismith::dsp::math::BackwardDifference differentiator;
  MockContext ctx;

  SECTION("The Transform() method") {
    SECTION("When called for the first time (Cold Start)") {
      SECTION("Should initialize history and return 0.0") {
        ctx.timestamp_ticks = 1000u;
        const float input = 1.0f;

        const float output = differentiator.Transform(input, ctx);

        REQUIRE_THAT(output, WithinAbs(0.0f, 0.000001f));
      }
    }

    SECTION("When the second sample arrives") {
      SECTION("Should calculate backward-difference speed in units per tick") {
        ctx.timestamp_ticks = 1000u;
        (void) differentiator.Transform(1.0f, ctx);

        ctx.timestamp_ticks = 1010u;
        const float input = 1.1f;

        const float output = differentiator.Transform(input, ctx);

        REQUIRE_THAT(output, WithinAbs(0.01f, 0.000001f));
      }
    }

    SECTION("When called repeatedly with constant increments") {
      SECTION("Should keep returning the same speed in units per tick") {
        ctx.timestamp_ticks = 1000u;
        (void) differentiator.Transform(1.0f, ctx);
        ctx.timestamp_ticks = 1010u;
        (void) differentiator.Transform(1.1f, ctx);

        ctx.timestamp_ticks = 1020u;
        const float output = differentiator.Transform(1.2f, ctx);

        REQUIRE_THAT(output, WithinAbs(0.01f, 0.000001f));
      }
    }

    SECTION("When two consecutive samples have the same timestamp") {
      SECTION("Should return 0.0 to avoid division by zero") {
        ctx.timestamp_ticks = 1000u;
        (void) differentiator.Transform(1.0f, ctx);

        ctx.timestamp_ticks = 1000u;
        const float output = differentiator.Transform(1.2f, ctx);

        REQUIRE_THAT(output, WithinAbs(0.0f, 0.000001f));
      }
    }

    SECTION("When timestamp ticks wrap around on 32 bits") {
      SECTION("Should compute speed using modular delta ticks") {
        ctx.timestamp_ticks = 0xFFFFFFF8u;
        (void) differentiator.Transform(1.0f, ctx);

        ctx.timestamp_ticks = 0x00000002u;
        const float output = differentiator.Transform(1.3f, ctx);

        REQUIRE_THAT(output, WithinAbs(0.03f, 0.000001f));
      }
    }
  }

  SECTION("The Reset() method") {
    SECTION("After a few samples have been processed") {
      SECTION("Should reset the internal state and trigger a cold start") {
        ctx.timestamp_ticks = 1000u;
        (void) differentiator.Transform(1.0f, ctx);

        differentiator.Reset();
        ctx.timestamp_ticks = 1010u;

        const float velocity = differentiator.Transform(1.0f, ctx);

        REQUIRE_THAT(velocity, WithinAbs(0.0f, 0.000001f));
      }
    }
  }
}

#endif

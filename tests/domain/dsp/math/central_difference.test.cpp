#if defined(UNIT_TESTS)

#include "domain/dsp/math/central_difference.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cstdint>

using Catch::Matchers::WithinAbs;

namespace {
struct MockContext {
  std::uint32_t timestamp_ticks = 0;
};
}  // namespace

TEST_CASE("The CentralDifference class") {
  constexpr float kFs = 1000.0f;
  domain::dsp::math::CentralDifference<kFs> differentiator;
  MockContext ctx;

  SECTION("The Transform() method") {
    SECTION("When called for the first time (Cold Start)") {
      SECTION("Should return 0.0 and initialize history") {
        ctx.timestamp_ticks = 1000;
        const float input = 1.0f;

        const float output = differentiator.Transform(input, ctx);

        REQUIRE_THAT(output, WithinAbs(0.0f, 0.001f));
      }
    }

    SECTION("When the second sample arrives") {
      SECTION("Should compute central-difference speed from current and second-last samples") {
        ctx.timestamp_ticks = 1000;
        (void) differentiator.Transform(1.0f, ctx);

        ctx.timestamp_ticks = 1010;
        const float input = 1.2f;

        const float output = differentiator.Transform(input, ctx);

        REQUIRE_THAT(output, WithinAbs(100.0f, 0.001f));
      }
    }

    SECTION("When called multiple times in steady state") {
      SECTION("Should keep producing the expected speed") {
        ctx.timestamp_ticks = 1000;
        (void) differentiator.Transform(1.0f, ctx);
        ctx.timestamp_ticks = 1010;
        (void) differentiator.Transform(1.2f, ctx);

        ctx.timestamp_ticks = 1020;
        const float output = differentiator.Transform(1.4f, ctx);

        REQUIRE_THAT(output, WithinAbs(200.0f, 0.001f));
      }
    }
  }

  SECTION("The Reset() method") {
    SECTION("After processing a few samples") {
      SECTION("Should reset the internal state and trigger a cold start") {
        ctx.timestamp_ticks = 1000;
        (void) differentiator.Transform(1.0f, ctx);

        differentiator.Reset();
        ctx.timestamp_ticks = 1010;

        const float velocity = differentiator.Transform(1.0f, ctx);

        REQUIRE_THAT(velocity, WithinAbs(0.0f, 0.001f));
      }
    }
  }
}

#endif

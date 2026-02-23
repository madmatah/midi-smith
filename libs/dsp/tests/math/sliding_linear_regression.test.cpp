#if defined(UNIT_TESTS)

#include "dsp/math/sliding_linear_regression.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cstddef>
#include <cstdint>

#include "dsp/concepts.hpp"

namespace {

struct TestContext {
  std::uint32_t timestamp_ticks = 0u;
};

using midismith::dsp::math::SlidingLinearRegression;

}  // namespace

TEST_CASE("The SlidingLinearRegression class") {
  using Catch::Matchers::WithinAbs;

  static_assert(
      midismith::dsp::concepts::SignalTransformer<SlidingLinearRegression<>, TestContext>);

  SECTION("Returns zero until there are enough samples") {
    SlidingLinearRegression<> estimator;
    TestContext ctx{1000u};

    REQUIRE_THAT(estimator.Transform(0.0f, ctx), WithinAbs(0.0f, 0.0001f));
  }

  SECTION("Computes the expected slope on a linear ramp") {
    SlidingLinearRegression<> estimator;
    TestContext ctx{1000u};

    REQUIRE_THAT(estimator.Transform(0.0f, ctx), WithinAbs(0.0f, 0.0001f));

    ctx.timestamp_ticks = 2000u;
    REQUIRE_THAT(estimator.Transform(2.0f, ctx), WithinAbs(0.002f, 0.000001f));

    ctx.timestamp_ticks = 3000u;
    REQUIRE_THAT(estimator.Transform(4.0f, ctx), WithinAbs(0.002f, 0.000001f));

    ctx.timestamp_ticks = 4000u;
    REQUIRE_THAT(estimator.Transform(6.0f, ctx), WithinAbs(0.002f, 0.000001f));
  }

  SECTION("Keeps the expected estimate with a sliding window") {
    SlidingLinearRegression<3u> estimator;
    TestContext ctx{1000u};

    (void) estimator.Transform(0.0f, ctx);
    ctx.timestamp_ticks = 2000u;
    (void) estimator.Transform(1.0f, ctx);
    ctx.timestamp_ticks = 3000u;
    REQUIRE_THAT(estimator.Transform(2.0f, ctx), WithinAbs(0.001f, 0.000001f));

    ctx.timestamp_ticks = 4000u;
    REQUIRE_THAT(estimator.Transform(3.0f, ctx), WithinAbs(0.001f, 0.000001f));

    ctx.timestamp_ticks = 5000u;
    REQUIRE_THAT(estimator.Transform(4.0f, ctx), WithinAbs(0.001f, 0.000001f));
  }

  SECTION("Handles 32-bit timestamp wrap-around") {
    SlidingLinearRegression<> estimator;
    const std::uint32_t t0 = 0xFFFFFF00u;
    const std::uint32_t t1 = static_cast<std::uint32_t>(t0 + 500u);
    const std::uint32_t t2 = static_cast<std::uint32_t>(t1 + 500u);
    TestContext ctx{t0};

    REQUIRE_THAT(estimator.Transform(0.0f, ctx), WithinAbs(0.0f, 0.0001f));

    ctx.timestamp_ticks = t1;
    REQUIRE_THAT(estimator.Transform(1.0f, ctx), WithinAbs(0.002f, 0.000001f));

    ctx.timestamp_ticks = t2;
    REQUIRE_THAT(estimator.Transform(2.0f, ctx), WithinAbs(0.002f, 0.000001f));
  }

  SECTION("Does not drift at rest over long duration") {
    SlidingLinearRegression<> estimator;
    TestContext ctx{1000u};

    float output = 0.0f;
    constexpr float kRestPosition = 0.55f;

    for (std::size_t i = 0u; i < 12000u; ++i) {
      output = estimator.Transform(kRestPosition, ctx);
      ctx.timestamp_ticks =
          static_cast<std::uint32_t>(ctx.timestamp_ticks + ((i & 1u) == 0u ? 286u : 285u));
    }

    REQUIRE_THAT(output, WithinAbs(0.0f, 0.001f));
  }

  SECTION("Reset clears history and sums") {
    SlidingLinearRegression<> estimator;
    TestContext ctx{1000u};

    (void) estimator.Transform(0.0f, ctx);
    ctx.timestamp_ticks = 2000u;
    REQUIRE_THAT(estimator.Transform(2.0f, ctx), WithinAbs(0.002f, 0.000001f));

    estimator.Reset();

    ctx.timestamp_ticks = 3000u;
    REQUIRE_THAT(estimator.Transform(10.0f, ctx), WithinAbs(0.0f, 0.0001f));

    ctx.timestamp_ticks = 4000u;
    REQUIRE_THAT(estimator.Transform(13.0f, ctx), WithinAbs(0.003f, 0.000001f));
  }
}

#endif

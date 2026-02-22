#if defined(UNIT_TESTS)

#include "domain/dsp/filters/simple_moving_average.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "domain/dsp/concepts.hpp"

namespace {

struct TestContext {};

using MovingAverage4 = midismith::adc_board::domain::dsp::filters::SimpleMovingAverage<4u>;
static_assert(
    midismith::adc_board::domain::dsp::concepts::SignalTransformer<MovingAverage4, TestContext>);

}  // namespace

TEST_CASE("The SimpleMovingAverage class") {
  using Catch::Matchers::WithinAbs;

  SECTION("The Transform() method") {
    SECTION("When configured with a window size of 1") {
      SECTION("Should return the last input sample") {
        midismith::adc_board::domain::dsp::filters::SimpleMovingAverage<1u> filter;
        TestContext context{};

        REQUIRE_THAT(filter.Transform(10.0f, context), WithinAbs(10.0f, 0.0001f));
        REQUIRE_THAT(filter.Transform(20.0f, context), WithinAbs(20.0f, 0.0001f));
        REQUIRE_THAT(filter.Transform(-5.0f, context), WithinAbs(-5.0f, 0.0001f));
      }
    }

    SECTION("When configured with a window size of 4") {
      SECTION("Should return the average of available samples during warmup") {
        MovingAverage4 filter;
        TestContext context{};

        REQUIRE_THAT(filter.Transform(10.0f, context), WithinAbs(10.0f, 0.0001f));
        REQUIRE_THAT(filter.Transform(20.0f, context), WithinAbs(15.0f, 0.0001f));
        REQUIRE_THAT(filter.Transform(30.0f, context), WithinAbs(20.0f, 0.0001f));
        REQUIRE_THAT(filter.Transform(40.0f, context), WithinAbs(25.0f, 0.0001f));
      }
    }

    SECTION("When configured with a window size of 3") {
      SECTION("Should keep only the latest samples after warmup") {
        midismith::adc_board::domain::dsp::filters::SimpleMovingAverage<3u> filter;
        TestContext context{};

        REQUIRE_THAT(filter.Transform(3.0f, context), WithinAbs(3.0f, 0.0001f));
        REQUIRE_THAT(filter.Transform(6.0f, context), WithinAbs(4.5f, 0.0001f));
        REQUIRE_THAT(filter.Transform(9.0f, context), WithinAbs(6.0f, 0.0001f));
        REQUIRE_THAT(filter.Transform(12.0f, context), WithinAbs(9.0f, 0.0001f));
      }
    }
  }

  SECTION("The Reset() method") {
    SECTION("When called after receiving samples") {
      SECTION("Should restore raw fallback behavior when empty") {
        MovingAverage4 filter;
        TestContext context{};

        REQUIRE_THAT(filter.Transform(100.0f, context), WithinAbs(100.0f, 0.0001f));
        filter.Reset();

        REQUIRE_THAT(filter.Compute(42.0f, context), WithinAbs(42.0f, 0.0001f));
      }
    }
  }
}

#endif

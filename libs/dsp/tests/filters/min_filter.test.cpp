#if defined(UNIT_TESTS)

#include "dsp/filters/min_filter.hpp"

#include <catch2/catch_test_macros.hpp>

#include "dsp/concepts.hpp"

namespace {

struct TestContext {};

static_assert(midismith::dsp::concepts::SignalTransformer<midismith::dsp::filters::MinFilter, TestContext>);
static_assert(midismith::dsp::concepts::Resettable<midismith::dsp::filters::MinFilter>);

}  // namespace

TEST_CASE("The MinFilter class") {
  SECTION("The Transform() method") {
    SECTION("When receiving mixed samples") {
      SECTION("Should accumulate and return the running minimum") {
        midismith::dsp::filters::MinFilter filter;
        TestContext context{};

        REQUIRE(filter.Transform(5.0f, context) == 5.0f);
        REQUIRE(filter.Transform(3.0f, context) == 3.0f);
        REQUIRE(filter.Transform(4.0f, context) == 3.0f);
        REQUIRE(filter.Transform(1.0f, context) == 1.0f);
        REQUIRE(filter.Transform(2.0f, context) == 1.0f);
      }
    }

    SECTION("When receiving strictly increasing samples") {
      SECTION("Should never increase the accumulated minimum") {
        midismith::dsp::filters::MinFilter filter;
        TestContext context{};

        REQUIRE(filter.Transform(1.0f, context) == 1.0f);
        REQUIRE(filter.Transform(3.0f, context) == 1.0f);
        REQUIRE(filter.Transform(5.0f, context) == 1.0f);
      }
    }

    SECTION("When receiving a single sample") {
      SECTION("Should return that sample") {
        midismith::dsp::filters::MinFilter filter;
        TestContext context{};

        REQUIRE(filter.Transform(7.0f, context) == 7.0f);
      }
    }

    SECTION("When all samples are identical") {
      SECTION("Should always return that value") {
        midismith::dsp::filters::MinFilter filter;
        TestContext context{};

        REQUIRE(filter.Transform(4.0f, context) == 4.0f);
        REQUIRE(filter.Transform(4.0f, context) == 4.0f);
        REQUIRE(filter.Transform(4.0f, context) == 4.0f);
        REQUIRE(filter.Transform(4.0f, context) == 4.0f);
        REQUIRE(filter.Transform(4.0f, context) == 4.0f);
      }
    }
  }

  SECTION("The Reset() method") {
    SECTION("When called after accumulation") {
      SECTION("Should discard the previous minimum so the next sample becomes the new minimum") {
        midismith::dsp::filters::MinFilter filter;
        TestContext context{};

        filter.Transform(0.5f, context);
        filter.Reset();

        REQUIRE(filter.Transform(10.0f, context) == 10.0f);
      }
    }
  }
}

#endif

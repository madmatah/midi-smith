#if defined(UNIT_TESTS)

#include "dsp/filters/max_filter.hpp"

#include <catch2/catch_test_macros.hpp>

#include "dsp/concepts.hpp"

namespace {

struct TestContext {};

static_assert(midismith::dsp::concepts::SignalTransformer<midismith::dsp::filters::MaxFilter, TestContext>);
static_assert(midismith::dsp::concepts::Resettable<midismith::dsp::filters::MaxFilter>);

}  // namespace

TEST_CASE("The MaxFilter class") {
  SECTION("The Transform() method") {
    SECTION("When receiving mixed samples") {
      SECTION("Should accumulate and return the running maximum") {
        midismith::dsp::filters::MaxFilter filter;
        TestContext context{};

        REQUIRE(filter.Transform(1.0f, context) == 1.0f);
        REQUIRE(filter.Transform(3.0f, context) == 3.0f);
        REQUIRE(filter.Transform(2.0f, context) == 3.0f);
        REQUIRE(filter.Transform(5.0f, context) == 5.0f);
        REQUIRE(filter.Transform(4.0f, context) == 5.0f);
      }
    }

    SECTION("When receiving strictly decreasing samples") {
      SECTION("Should never decrease the accumulated maximum") {
        midismith::dsp::filters::MaxFilter filter;
        TestContext context{};

        REQUIRE(filter.Transform(5.0f, context) == 5.0f);
        REQUIRE(filter.Transform(3.0f, context) == 5.0f);
        REQUIRE(filter.Transform(1.0f, context) == 5.0f);
      }
    }

    SECTION("When receiving a single sample") {
      SECTION("Should return that sample") {
        midismith::dsp::filters::MaxFilter filter;
        TestContext context{};

        REQUIRE(filter.Transform(7.0f, context) == 7.0f);
      }
    }

    SECTION("When all samples are identical") {
      SECTION("Should always return that value") {
        midismith::dsp::filters::MaxFilter filter;
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
      SECTION("Should discard the previous maximum so the next sample becomes the new maximum") {
        midismith::dsp::filters::MaxFilter filter;
        TestContext context{};

        filter.Transform(10.0f, context);
        filter.Reset();

        REQUIRE(filter.Transform(2.0f, context) == 2.0f);
      }
    }
  }
}

#endif

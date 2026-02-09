#if defined(UNIT_TESTS)

#include "domain/dsp/filters/ema_filter.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cstdint>

namespace {
struct TestContext {};
}  // namespace

TEST_CASE("The EmaFilterRatio class") {
  SECTION("The Transform() method") {
    SECTION("When alpha is 1") {
      SECTION("Should return the input sample") {
        domain::dsp::filters::EmaFilterRatio<1, 1> filter;
        TestContext ctx{};
        using Catch::Matchers::WithinRel;

        REQUIRE_THAT(filter.Transform(100.0f, ctx), WithinRel(100.0f));
        REQUIRE_THAT(filter.Transform(200.0f, ctx), WithinRel(200.0f));
        REQUIRE_THAT(filter.Transform(1234.0f, ctx), WithinRel(1234.0f));
        REQUIRE_THAT(filter.Transform(65535.0f, ctx), WithinRel(65535.0f));
      }
    }

    SECTION("When alpha is 0") {
      SECTION("Should keep the first value") {
        domain::dsp::filters::EmaFilterRatio<0, 1> filter;
        TestContext ctx{};
        using Catch::Matchers::WithinRel;

        REQUIRE_THAT(filter.Transform(1234.0f, ctx), WithinRel(1234.0f));
        REQUIRE_THAT(filter.Transform(4321.0f, ctx), WithinRel(1234.0f));
        REQUIRE_THAT(filter.Transform(0.0f, ctx), WithinRel(1234.0f));
      }
    }

    SECTION("When alpha is between 0 and 1") {
      SECTION("Should converge monotonically to a step without overshoot") {
        domain::dsp::filters::EmaFilterRatio<1, 4> filter;
        TestContext ctx{};

        using Catch::Matchers::WithinAbs;
        REQUIRE_THAT(filter.Transform(0.0f, ctx), WithinAbs(0.0f, 0.0001f));

        float prev = filter.Transform(1000.0f, ctx);
        REQUIRE(prev <= 1000.0f);

        for (std::uint32_t i = 0; i < 20; ++i) {
          const float now = filter.Transform(1000.0f, ctx);
          REQUIRE(now >= prev);
          REQUIRE(now <= 1000.0f);
          prev = now;
        }
      }
    }
  }

  SECTION("The Reset() method") {
    SECTION("When called after receiving samples") {
      SECTION("Should restore the raw fallback behavior") {
        domain::dsp::filters::EmaFilterRatio<1, 2> filter;
        TestContext ctx{};

        REQUIRE_THAT(filter.Transform(1111.0f, ctx), Catch::Matchers::WithinRel(1111.0f));
        filter.Reset();

        REQUIRE_THAT(filter.Compute(2222.0f, ctx), Catch::Matchers::WithinRel(2222.0f));
      }
    }
  }
}

#endif

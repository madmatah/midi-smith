#if defined(UNIT_TESTS)

#include "dsp/filters/sg5_smoother.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cstdint>

namespace {
struct TestContext {};
}  // namespace

TEST_CASE("The Sg5Smoother class") {
  using Catch::Matchers::WithinAbs;

  SECTION("The Transform() method") {
    SECTION("When called with a constant signal") {
      SECTION("Should return the same constant after warmup") {
        midismith::dsp::filters::Sg5Smoother filter;
        TestContext ctx{};

        for (std::uint32_t i = 0; i < 10; ++i) {
          REQUIRE_THAT(filter.Transform(1234.0f, ctx), WithinAbs(1234.0f, 0.001f));
        }
      }
    }

    SECTION("When called with a ramp signal") {
      SECTION("Should return the newest sample after warmup") {
        midismith::dsp::filters::Sg5Smoother filter;
        TestContext ctx{};

        REQUIRE_THAT(filter.Transform(10.0f, ctx), WithinAbs(10.0f, 0.001f));
        REQUIRE_THAT(filter.Transform(20.0f, ctx), WithinAbs(20.0f, 0.001f));
        REQUIRE_THAT(filter.Transform(30.0f, ctx), WithinAbs(30.0f, 0.001f));
        REQUIRE_THAT(filter.Transform(40.0f, ctx), WithinAbs(40.0f, 0.001f));
        REQUIRE_THAT(filter.Transform(50.0f, ctx), WithinAbs(50.0f, 0.001f));
        REQUIRE_THAT(filter.Transform(60.0f, ctx), WithinAbs(60.0f, 0.001f));
      }
    }
  }
}

#endif

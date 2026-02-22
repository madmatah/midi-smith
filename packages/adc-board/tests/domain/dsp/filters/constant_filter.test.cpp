#if defined(UNIT_TESTS)

#include "domain/dsp/filters/constant_filter.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

namespace {

struct TestContext {};

}  // namespace

TEST_CASE("The ConstantFilter class") {
  using Catch::Matchers::WithinRel;
  using midismith::adc_board::domain::dsp::filters::ConstantFilter;

  SECTION("The Transform() method") {
    SECTION("Should always return the configured constant value") {
      ConstantFilter<0.5f> filter;
      TestContext ctx{};

      REQUIRE_THAT(filter.Transform(-10.0f, ctx), WithinRel(0.5f));
      REQUIRE_THAT(filter.Transform(0.0f, ctx), WithinRel(0.5f));
      REQUIRE_THAT(filter.Transform(42.0f, ctx), WithinRel(0.5f));
    }
  }

  SECTION("The Reset() method") {
    SECTION("Should be callable without side effects") {
      ConstantFilter<0.5f> filter;

      filter.Reset();

      REQUIRE(true);
    }
  }
}

#endif

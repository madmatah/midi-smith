#if defined(UNIT_TESTS)

#include "domain/dsp/converters/linear_scaler.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

namespace {

struct TestContext {
  int unused = 0;
};

}  // namespace

TEST_CASE("The LinearScaler class", "[domain][dsp][converters]") {
  using Catch::Matchers::WithinAbs;

  midismith::adc_board::domain::dsp::converters::LinearScaler<2.5f> scaler;
  TestContext ctx{};

  REQUIRE_THAT(scaler.Transform(0.0f, ctx), WithinAbs(0.0f, 0.0001f));
  REQUIRE_THAT(scaler.Transform(1.2f, ctx), WithinAbs(3.0f, 0.0001f));
  REQUIRE_THAT(scaler.Transform(-4.0f, ctx), WithinAbs(-10.0f, 0.0001f));
}

#endif

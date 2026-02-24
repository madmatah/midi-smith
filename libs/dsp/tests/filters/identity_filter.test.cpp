#if defined(UNIT_TESTS)
#include "dsp/filters/identity_filter.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cstdint>

namespace {
struct TestContext {};
}  // namespace

TEST_CASE("The IdentityFilter class") {
  using Catch::Matchers::WithinRel;

  SECTION("The Transform() method") {
    SECTION("When called with any input value") {
      SECTION("Should return the input value unchanged") {
        midismith::dsp::filters::IdentityFilter filter;
        TestContext ctx{};

        REQUIRE_THAT(filter.Transform(0.0f, ctx), WithinRel(0.0f));
        REQUIRE_THAT(filter.Transform(1.0f, ctx), WithinRel(1.0f));
        REQUIRE_THAT(filter.Transform(42.0f, ctx), WithinRel(42.0f));
        REQUIRE_THAT(filter.Transform(65535.0f, ctx), WithinRel(65535.0f));
      }
    }
  }
}
#endif

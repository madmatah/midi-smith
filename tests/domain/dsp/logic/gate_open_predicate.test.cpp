#if defined(UNIT_TESTS)

#include "domain/dsp/logic/gate_open_predicate.hpp"

#include <catch2/catch_test_macros.hpp>

namespace {

struct MockContext {
  float value = 0.0f;
};

float ReadValue(const MockContext& ctx) noexcept {
  return ctx.value;
}

}  // namespace

TEST_CASE("The GateOpenPredicate struct") {
  using domain::dsp::logic::GateOpenPredicate;
  using Predicate = GateOpenPredicate<0.45f, ReadValue>;

  SECTION("The Test() method") {
    SECTION("When the value is below threshold") {
      MockContext ctx{.value = 0.44f};

      REQUIRE(Predicate::Test(ctx));
    }

    SECTION("When the value equals threshold") {
      MockContext ctx{.value = 0.45f};

      REQUIRE_FALSE(Predicate::Test(ctx));
    }

    SECTION("When the value is above threshold") {
      MockContext ctx{.value = 0.46f};

      REQUIRE_FALSE(Predicate::Test(ctx));
    }
  }
}

#endif

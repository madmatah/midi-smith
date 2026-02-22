#if defined(UNIT_TESTS)

#include "domain/dsp/logic/switch.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

namespace {

struct TestContext {};

struct AlwaysTrue {
  static bool Test(const TestContext& ctx) noexcept {
    (void) ctx;
    return true;
  }
};

struct AlwaysFalse {
  static bool Test(const TestContext& ctx) noexcept {
    (void) ctx;
    return false;
  }
};

class TrueStage {
 public:
  void Reset() noexcept {
    reset_count++;
  }

  float Transform(float sample, const TestContext& ctx) noexcept {
    (void) ctx;
    return sample + 1.0f;
  }

  static void ResetCounts() noexcept {
    reset_count = 0;
  }

  static inline int reset_count = 0;
};

class FalseStage {
 public:
  void Reset() noexcept {
    reset_count++;
  }

  float Transform(float sample, const TestContext& ctx) noexcept {
    (void) ctx;
    return sample - 1.0f;
  }

  static void ResetCounts() noexcept {
    reset_count = 0;
  }

  static inline int reset_count = 0;
};

}  // namespace

TEST_CASE("The Switch class") {
  using Catch::Matchers::WithinAbs;
  using midismith::adc_board::domain::dsp::logic::Switch;

  SECTION("The Transform() method") {
    TestContext ctx{};

    SECTION("When predicate evaluates to true") {
      Switch<AlwaysTrue, TrueStage, FalseStage> stage;

      REQUIRE_THAT(stage.Transform(10.0f, ctx), WithinAbs(11.0f, 0.001f));
    }

    SECTION("When predicate evaluates to false") {
      Switch<AlwaysFalse, TrueStage, FalseStage> stage;

      REQUIRE_THAT(stage.Transform(10.0f, ctx), WithinAbs(9.0f, 0.001f));
    }
  }

  SECTION("The Reset() method") {
    Switch<AlwaysTrue, TrueStage, FalseStage> stage;
    TrueStage::ResetCounts();
    FalseStage::ResetCounts();

    stage.Reset();

    REQUIRE(TrueStage::reset_count == 1);
    REQUIRE(FalseStage::reset_count == 1);
  }

  SECTION("The branch accessors") {
    Switch<AlwaysTrue, TrueStage, FalseStage> stage;

    auto& true_stage = stage.TrueStage();
    auto& true_stage_again = stage.TrueStage();
    REQUIRE(&true_stage == &true_stage_again);

    auto& false_stage = stage.FalseStage();
    auto& false_stage_again = stage.FalseStage();
    REQUIRE(&false_stage == &false_stage_again);
  }
}

#endif

#if defined(UNIT_TESTS)

#include "domain/dsp/logic/or.hpp"

#include <catch2/catch_test_macros.hpp>

namespace {

struct MockContext {
  bool val1 = false;
  bool val2 = false;
  bool val3 = false;
  mutable int call_count1 = 0;
  mutable int call_count2 = 0;
  mutable int call_count3 = 0;
};

struct Pred1 {
  static bool Test(const MockContext& ctx) noexcept {
    ctx.call_count1++;
    return ctx.val1;
  }
};

struct Pred2 {
  static bool Test(const MockContext& ctx) noexcept {
    ctx.call_count2++;
    return ctx.val2;
  }
};

struct Pred3 {
  static bool Test(const MockContext& ctx) noexcept {
    ctx.call_count3++;
    return ctx.val3;
  }
};

}  // namespace

TEST_CASE("The Or struct") {
  using midismith::adc_board::domain::dsp::logic::Or;

  SECTION("The Test() method") {
    SECTION("With two predicates") {
      using Pred = Or<Pred1, Pred2>;

      SECTION("When both are false") {
        MockContext ctx{.val1 = false, .val2 = false};
        REQUIRE_FALSE(Pred::Test(ctx));
      }

      SECTION("When first is true") {
        MockContext ctx{.val1 = true, .val2 = false};
        REQUIRE(Pred::Test(ctx));
      }

      SECTION("When second is true") {
        MockContext ctx{.val1 = false, .val2 = true};
        REQUIRE(Pred::Test(ctx));
      }
    }

    SECTION("With three predicates") {
      using Pred = Or<Pred1, Pred2, Pred3>;

      SECTION("When all are false") {
        MockContext ctx{.val1 = false, .val2 = false, .val3 = false};
        REQUIRE_FALSE(Pred::Test(ctx));
      }

      SECTION("When one is true") {
        MockContext ctx{.val1 = false, .val2 = true, .val3 = false};
        REQUIRE(Pred::Test(ctx));
      }
    }

    SECTION("Short-circuiting behavior") {
      using Pred = Or<Pred1, Pred2, Pred3>;

      SECTION("Should not call subsequent predicates if first is true") {
        MockContext ctx{.val1 = true, .val2 = false, .val3 = false};

        Pred::Test(ctx);

        REQUIRE(ctx.call_count1 == 1);
        REQUIRE(ctx.call_count2 == 0);
        REQUIRE(ctx.call_count3 == 0);
      }
    }
  }
}

#endif

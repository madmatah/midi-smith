#if defined(UNIT_TESTS)

#include "domain/dsp/logic/is_false.hpp"

#include <catch2/catch_test_macros.hpp>

namespace {

struct MockContext {
  bool value = false;
  int int_value = 0;
};

bool ReadBool(const MockContext& ctx) noexcept {
  return ctx.value;
}

int ReadInt(const MockContext& ctx) noexcept {
  return ctx.int_value;
}

}  // namespace

TEST_CASE("The IsFalse struct") {
  using domain::dsp::logic::IsFalse;

  SECTION("The Test() method") {
    SECTION("When the value provider returns a bool") {
      using Pred = IsFalse<ReadBool>;

      SECTION("When value is true") {
        MockContext ctx{.value = true};
        REQUIRE_FALSE(Pred::Test(ctx));
      }

      SECTION("When value is false") {
        MockContext ctx{.value = false};
        REQUIRE(Pred::Test(ctx));
      }
    }

    SECTION("When the value provider returns an int (implicit conversion)") {
      using Pred = IsFalse<ReadInt>;

      SECTION("When value is non-zero") {
        MockContext ctx{.int_value = 42};
        REQUIRE_FALSE(Pred::Test(ctx));
      }

      SECTION("When value is zero") {
        MockContext ctx{.int_value = 0};
        REQUIRE(Pred::Test(ctx));
      }
    }
  }
}

#endif

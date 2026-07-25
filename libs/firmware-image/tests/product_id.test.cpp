#if defined(UNIT_TESTS)

#include "firmware-image/product_id.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cstdint>

namespace {

using midismith::firmware_image::MakeProductId;
using midismith::firmware_image::ProductId;

constexpr std::uint16_t kMainBoardRawValue = 1;
constexpr std::uint16_t kAdcBoardRawValue = 2;
constexpr std::uint16_t kUnassignedRawValue = 0x4242;

}  // namespace

TEST_CASE("The MakeProductId function") {
  SECTION("When the raw value names the main board") {
    SECTION("Should return kMainBoard so main board images stay installable") {
      REQUIRE(MakeProductId(kMainBoardRawValue) == ProductId::kMainBoard);
    }
  }

  SECTION("When the raw value names an ADC board") {
    SECTION("Should return kAdcBoard so ADC board images stay installable") {
      REQUIRE(MakeProductId(kAdcBoardRawValue) == ProductId::kAdcBoard);
    }
  }

  SECTION("When the raw value names no product this firmware knows") {
    SECTION("Should collapse to kUnknown so it can never match a target") {
      REQUIRE(MakeProductId(kUnassignedRawValue) == ProductId::kUnknown);
    }
  }

  SECTION("When the raw value is the reserved zero") {
    SECTION("Should stay kUnknown rather than name a product by accident") {
      REQUIRE(MakeProductId(0) == ProductId::kUnknown);
    }
  }

  SECTION("When every known product is mapped") {
    SECTION("Should give each one a distinct identifier") {
      REQUIRE(MakeProductId(kMainBoardRawValue) != MakeProductId(kAdcBoardRawValue));
    }
  }
}

#endif

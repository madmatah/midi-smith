#if defined(UNIT_TESTS)

#include "domain/music/piano/velocity/constant_velocity_mapper.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("The ConstantVelocityMapper class") {
  SECTION("The Map() method") {
    SECTION("When configured with velocity 64") {
      SECTION("Should return 64 for negative, zero and positive speeds") {
        domain::music::piano::velocity::ConstantVelocityMapper<64u> mapper{};

        const domain::music::Velocity mapped_negative = mapper.Map(-1.0f);
        const domain::music::Velocity mapped_zero = mapper.Map(0.0f);
        const domain::music::Velocity mapped_positive = mapper.Map(1.0f);

        REQUIRE(mapped_negative == static_cast<domain::music::Velocity>(64u));
        REQUIRE(mapped_zero == static_cast<domain::music::Velocity>(64u));
        REQUIRE(mapped_positive == static_cast<domain::music::Velocity>(64u));
      }
    }

    SECTION("When configured with velocity 127") {
      SECTION("Should return 127 for negative, zero and positive speeds") {
        domain::music::piano::velocity::ConstantVelocityMapper<127u> mapper{};

        const domain::music::Velocity mapped_negative = mapper.Map(-0.5f);
        const domain::music::Velocity mapped_zero = mapper.Map(0.0f);
        const domain::music::Velocity mapped_positive = mapper.Map(0.5f);

        REQUIRE(mapped_negative == static_cast<domain::music::Velocity>(127u));
        REQUIRE(mapped_zero == static_cast<domain::music::Velocity>(127u));
        REQUIRE(mapped_positive == static_cast<domain::music::Velocity>(127u));
      }
    }
  }
}

#endif

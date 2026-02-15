#if defined(UNIT_TESTS)

#include "domain/music/piano/goebl_logarithmic_velocity_mapper.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("GoeblLogarithmicVelocityMapper") {
  domain::music::piano::GoeblLogarithmicVelocityMapper mapper{};

  SECTION("Clamps non-positive speed to 1") {
    REQUIRE(mapper.Map(0.0f) == static_cast<domain::music::Velocity>(1u));
    REQUIRE(mapper.Map(-1.0f) == static_cast<domain::music::Velocity>(1u));
  }

  SECTION("Maps 1.0 m/s to ~58") {
    REQUIRE(mapper.Map(1.0f) == static_cast<domain::music::Velocity>(58u));
  }

  SECTION("Clamps high speed to 127") {
    REQUIRE(mapper.Map(10.0f) == static_cast<domain::music::Velocity>(127u));
  }
}

#endif

#if defined(UNIT_TESTS)

#include "domain/music/piano/velocity/exponential_velocity_mapper.hpp"

#include <catch2/catch_test_macros.hpp>
#include <limits>

TEST_CASE("ExponentialVelocityMapper") {
  using Mapper =
      midismith::adc_board::domain::music::piano::velocity::ExponentialVelocityMapper<1.0f, 2.0f>;
  Mapper mapper{};

  SECTION("Returns 127 in fail-safe conditions") {
    REQUIRE(mapper.Map(0.0f) == static_cast<midismith::common::domain::music::Velocity>(127u));
    REQUIRE(mapper.Map(-0.2f) == static_cast<midismith::common::domain::music::Velocity>(127u));
    REQUIRE(mapper.Map(std::numeric_limits<float>::quiet_NaN()) ==
            static_cast<midismith::common::domain::music::Velocity>(127u));
    REQUIRE(mapper.Map(std::numeric_limits<float>::infinity()) ==
            static_cast<midismith::common::domain::music::Velocity>(127u));
  }

  SECTION("Maps configured maximum speed to 127") {
    REQUIRE(mapper.Map(1.0f) == static_cast<midismith::common::domain::music::Velocity>(127u));
  }

  SECTION("Clamps speed above configured maximum to 127") {
    REQUIRE(mapper.Map(2.0f) == static_cast<midismith::common::domain::music::Velocity>(127u));
  }

  SECTION("Maps an intermediate speed with exponential shaping") {
    REQUIRE(mapper.Map(0.5f) == static_cast<midismith::common::domain::music::Velocity>(32u));
  }
}

#endif

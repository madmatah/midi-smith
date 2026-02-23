#if defined(UNIT_TESTS)

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "sensor-linearization/cny70_response_curve.hpp"

TEST_CASE("The CNY70 master signature table") {
  using Catch::Matchers::WithinAbs;
  using midismith::sensor_linearization::Cny70DatasheetSensorResponseCurve;
  using midismith::sensor_linearization::kCny70DatasheetResponseCurve;

  const auto table = Cny70DatasheetSensorResponseCurve();

  SECTION("RelativeCurrentAtDistanceMm() matches the data points") {
    for (const auto& point : kCny70DatasheetResponseCurve) {
      REQUIRE_THAT(table.RelativeCurrentAtDistanceMm(point.distance_mm),
                   WithinAbs(point.relative_current, 0.0001f));
    }
  }

  SECTION("DistanceMmAtRelativeCurrent() inverts the data points") {
    for (const auto& point : kCny70DatasheetResponseCurve) {
      REQUIRE_THAT(table.DistanceMmAtRelativeCurrent(point.relative_current),
                   WithinAbs(point.distance_mm, 0.0001f));
    }
  }
}

#endif

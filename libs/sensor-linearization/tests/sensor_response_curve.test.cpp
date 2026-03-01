#if defined(UNIT_TESTS)

#include "sensor-linearization/sensor_response_curve.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "sensor-linearization/cny70_response_curve.hpp"

using Catch::Matchers::WithinAbs;
using midismith::sensor_linearization::SensorResponseCurve;
using midismith::sensor_linearization::SensorResponsePoint;

TEST_CASE("The SensorResponseCurve class", "[sensor][linearization]") {
  SECTION("With a monotone decreasing curve (e.g. Distance to Current)") {
    const SensorResponsePoint points[] = {
        {.distance_mm = 1.0f, .relative_current = 1.0f},
        {.distance_mm = 2.0f, .relative_current = 0.5f},
        {.distance_mm = 4.0f, .relative_current = 0.25f},
    };
    SensorResponseCurve curve(points, 3);

    SECTION("RelativeCurrentAtDistanceMm()") {
      SECTION("Should interpolate correctly between points") {
        REQUIRE_THAT(curve.RelativeCurrentAtDistanceMm(1.5f), WithinAbs(0.75f, 0.0001f));
        REQUIRE_THAT(curve.RelativeCurrentAtDistanceMm(3.0f), WithinAbs(0.375f, 0.0001f));
      }

      SECTION("Should extrapolate and clamp when outside range") {
        REQUIRE_THAT(curve.RelativeCurrentAtDistanceMm(0.5f), WithinAbs(1.0f, 0.0001f));
        REQUIRE_THAT(curve.RelativeCurrentAtDistanceMm(5.0f), WithinAbs(0.125f, 0.0001f));
      }
    }

    SECTION("DistanceMmAtRelativeCurrent()") {
      SECTION("Should invert interpolate correctly") {
        REQUIRE_THAT(curve.DistanceMmAtRelativeCurrent(0.75f), WithinAbs(1.5f, 0.0001f));
        REQUIRE_THAT(curve.DistanceMmAtRelativeCurrent(0.375f), WithinAbs(3.0f, 0.0001f));
      }
    }
  }

  SECTION("With a monotone increasing curve") {
    const SensorResponsePoint points[] = {
        {.distance_mm = 1.0f, .relative_current = 0.1f},
        {.distance_mm = 5.0f, .relative_current = 0.9f},
    };
    SensorResponseCurve curve(points, 2);

    SECTION("DistanceMmAtRelativeCurrent()") {
      SECTION("Should handle increasing slope correctly") {
        REQUIRE_THAT(curve.DistanceMmAtRelativeCurrent(0.5f), WithinAbs(3.0f, 0.0001f));
      }
    }
  }

  SECTION("With edge cases") {
    SECTION("When point count is less than 2") {
      SECTION("Should return 0.0f safely") {
        SensorResponseCurve empty(nullptr, 0);
        REQUIRE(empty.RelativeCurrentAtDistanceMm(1.0f) == 0.0f);
        REQUIRE(empty.DistanceMmAtRelativeCurrent(0.5f) == 0.0f);

        const SensorResponsePoint single_point[] = {
            {.distance_mm = 1.0f, .relative_current = 0.5f}};
        SensorResponseCurve single(single_point, 1);
        REQUIRE(single.RelativeCurrentAtDistanceMm(1.0f) == 0.0f);
        REQUIRE(single.DistanceMmAtRelativeCurrent(0.5f) == 0.0f);
      }
    }
  }
}

TEST_CASE("The CNY70 master signature table", "[sensor][linearization]") {
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

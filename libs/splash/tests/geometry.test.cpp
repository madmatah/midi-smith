#if defined(UNIT_TESTS)

#include "splash/geometry.hpp"

#include <array>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <numbers>

using Catch::Matchers::WithinAbs;
using midismith::splash::BezierPoints;
using midismith::splash::Point;
using midismith::splash::RotatePoint;
using midismith::splash::ScalePointsTowardCentroid;
using midismith::splash::TransformPoints;

TEST_CASE("The geometry functions") {
  SECTION("The RotatePoint() function") {
    SECTION("When rotating a quarter turn around the origin") {
      SECTION("Should map the x axis onto the y axis") {
        const Point rotated = RotatePoint({1.0, 0.0}, {0.0, 0.0}, std::numbers::pi_v<double> / 2.0);

        REQUIRE_THAT(rotated.x, WithinAbs(0.0, 1e-12));
        REQUIRE_THAT(rotated.y, WithinAbs(1.0, 1e-12));
      }
    }
    SECTION("When the pivot is not the origin") {
      SECTION("Should rotate around the pivot") {
        const Point rotated = RotatePoint({3.0, 2.0}, {2.0, 2.0}, std::numbers::pi_v<double>);

        REQUIRE_THAT(rotated.x, WithinAbs(1.0, 1e-12));
        REQUIRE_THAT(rotated.y, WithinAbs(2.0, 1e-12));
      }
    }
  }

  SECTION("The TransformPoints() function") {
    SECTION("When the angle is zero") {
      SECTION("Should keep every point in place") {
        const std::array<Point, 2> points{{{1.0, 2.0}, {-3.0, 4.0}}};

        const auto transformed = TransformPoints(points, {10.0, 10.0}, 0.0);

        REQUIRE_THAT(transformed[0].x, WithinAbs(1.0, 1e-12));
        REQUIRE_THAT(transformed[0].y, WithinAbs(2.0, 1e-12));
        REQUIRE_THAT(transformed[1].x, WithinAbs(-3.0, 1e-12));
        REQUIRE_THAT(transformed[1].y, WithinAbs(4.0, 1e-12));
      }
    }
  }

  SECTION("The ScalePointsTowardCentroid() function") {
    SECTION("When the factor is one") {
      SECTION("Should keep every point in place") {
        const std::array<Point, 1> points{{{5.0, 7.0}}};

        const auto scaled = ScalePointsTowardCentroid(points, {1.0, 1.0}, 1.0);

        REQUIRE_THAT(scaled[0].x, WithinAbs(5.0, 1e-12));
        REQUIRE_THAT(scaled[0].y, WithinAbs(7.0, 1e-12));
      }
    }
    SECTION("When the factor is one half") {
      SECTION("Should move every point halfway to the centroid") {
        const std::array<Point, 1> points{{{4.0, 0.0}}};

        const auto scaled = ScalePointsTowardCentroid(points, {0.0, 0.0}, 0.5);

        REQUIRE_THAT(scaled[0].x, WithinAbs(2.0, 1e-12));
        REQUIRE_THAT(scaled[0].y, WithinAbs(0.0, 1e-12));
      }
    }
  }

  SECTION("The BezierPoints() function") {
    SECTION("When sampling a cubic curve") {
      SECTION("Should start and end on the anchor points") {
        const auto points = BezierPoints<8>({0.0, 0.0}, {1.0, 2.0}, {3.0, 2.0}, {4.0, 0.0});

        REQUIRE(points.size() == 9);
        REQUIRE_THAT(points.front().x, WithinAbs(0.0, 1e-12));
        REQUIRE_THAT(points.front().y, WithinAbs(0.0, 1e-12));
        REQUIRE_THAT(points.back().x, WithinAbs(4.0, 1e-12));
        REQUIRE_THAT(points.back().y, WithinAbs(0.0, 1e-12));
      }
      SECTION("Should stay inside the control hull at the midpoint") {
        const auto points = BezierPoints<8>({0.0, 0.0}, {1.0, 2.0}, {3.0, 2.0}, {4.0, 0.0});

        REQUIRE_THAT(points[4].x, WithinAbs(2.0, 1e-12));
        REQUIRE_THAT(points[4].y, WithinAbs(1.5, 1e-12));
      }
    }
  }
}

#endif

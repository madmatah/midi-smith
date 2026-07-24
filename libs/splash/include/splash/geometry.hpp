#pragma once

#include <array>
#include <cmath>
#include <cstddef>

namespace midismith::splash {

struct Point {
  double x;
  double y;
};

inline Point RotatePoint(Point point, Point pivot, double angle_radians) {
  const double direction_x = point.x - pivot.x;
  const double direction_y = point.y - pivot.y;
  const double cosine = std::cos(angle_radians);
  const double sine = std::sin(angle_radians);
  return {pivot.x + cosine * direction_x - sine * direction_y,
          pivot.y + sine * direction_x + cosine * direction_y};
}

template <std::size_t kPointCount>
std::array<Point, kPointCount> TransformPoints(const std::array<Point, kPointCount>& points,
                                               Point pivot, double angle_radians) {
  std::array<Point, kPointCount> transformed{};
  for (std::size_t point_index = 0; point_index < kPointCount; ++point_index) {
    transformed[point_index] = RotatePoint(points[point_index], pivot, angle_radians);
  }
  return transformed;
}

template <std::size_t kPointCount>
constexpr std::array<Point, kPointCount> ScalePointsTowardCentroid(
    const std::array<Point, kPointCount>& points, Point centroid, double factor) {
  std::array<Point, kPointCount> scaled{};
  for (std::size_t point_index = 0; point_index < kPointCount; ++point_index) {
    scaled[point_index] = {centroid.x + (points[point_index].x - centroid.x) * factor,
                           centroid.y + (points[point_index].y - centroid.y) * factor};
  }
  return scaled;
}

template <std::size_t kSegmentCount>
constexpr std::array<Point, kSegmentCount + 1> BezierPoints(Point start, Point first_control,
                                                            Point second_control, Point end) {
  std::array<Point, kSegmentCount + 1> points{};
  for (std::size_t segment_index = 0; segment_index <= kSegmentCount; ++segment_index) {
    const double progress = static_cast<double>(segment_index) / static_cast<double>(kSegmentCount);
    const double inverse = 1.0 - progress;
    const double start_weight = inverse * inverse * inverse;
    const double first_control_weight = 3.0 * inverse * inverse * progress;
    const double second_control_weight = 3.0 * inverse * progress * progress;
    const double end_weight = progress * progress * progress;
    points[segment_index] = {start_weight * start.x + first_control_weight * first_control.x +
                                 second_control_weight * second_control.x + end_weight * end.x,
                             start_weight * start.y + first_control_weight * first_control.y +
                                 second_control_weight * second_control.y + end_weight * end.y};
  }
  return points;
}

}  // namespace midismith::splash

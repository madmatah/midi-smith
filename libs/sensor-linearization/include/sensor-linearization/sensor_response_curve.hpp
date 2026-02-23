#pragma once

#include <algorithm>
#include <cstddef>

namespace midismith::sensor_linearization {

struct SensorResponsePoint {
  float distance_mm = 0.0f;
  float relative_current = 0.0f;
};

class SensorResponseCurve {
 public:
  SensorResponseCurve(const SensorResponsePoint* points, std::size_t count) noexcept
      : points_(points), count_(count) {}

  std::size_t count() const noexcept {
    return count_;
  }

  float MinDistanceMm() const noexcept {
    if (points_ == nullptr || count_ == 0u) {
      return 0.0f;
    }
    return points_[0].distance_mm;
  }

  float MaxDistanceMm() const noexcept {
    if (points_ == nullptr || count_ == 0u) {
      return 0.0f;
    }
    return points_[count_ - 1u].distance_mm;
  }

  float RelativeCurrentAtDistanceMm(float distance_mm) const noexcept {
    if (points_ == nullptr || count_ < 2u) {
      return 0.0f;
    }

    const SensorResponsePoint& p0 = points_[0];
    const SensorResponsePoint& p1 = points_[1];
    const SensorResponsePoint& pn_1 = points_[count_ - 2u];
    const SensorResponsePoint& pn = points_[count_ - 1u];

    if (distance_mm <= p0.distance_mm) {
      return Clamp01(Interpolate(p0, p1, distance_mm));
    }
    if (distance_mm >= pn.distance_mm) {
      return Clamp01(Interpolate(pn_1, pn, distance_mm));
    }

    for (std::size_t i = 0; i + 1u < count_; ++i) {
      const SensorResponsePoint& a = points_[i];
      const SensorResponsePoint& b = points_[i + 1u];
      if (distance_mm >= a.distance_mm && distance_mm <= b.distance_mm) {
        return Clamp01(Interpolate(a, b, distance_mm));
      }
    }

    return Clamp01(pn.relative_current);
  }

  float DistanceMmAtRelativeCurrent(float relative_current) const noexcept {
    if (points_ == nullptr || count_ < 2u) {
      return 0.0f;
    }

    const float r = Clamp01(relative_current);
    const SensorResponsePoint& p0 = points_[0];
    const SensorResponsePoint& p1 = points_[1];
    const SensorResponsePoint& pn_1 = points_[count_ - 2u];
    const SensorResponsePoint& pn = points_[count_ - 1u];

    const bool is_monotone_decreasing = p0.relative_current >= pn.relative_current;
    if (is_monotone_decreasing) {
      if (r >= p0.relative_current) {
        return InvertInterpolate(p0, p1, r);
      }
      if (r <= pn.relative_current) {
        return InvertInterpolate(pn_1, pn, r);
      }
      for (std::size_t i = 0; i + 1u < count_; ++i) {
        const SensorResponsePoint& a = points_[i];
        const SensorResponsePoint& b = points_[i + 1u];
        if (r <= a.relative_current && r >= b.relative_current) {
          return InvertInterpolate(a, b, r);
        }
      }
    } else {
      if (r <= p0.relative_current) {
        return InvertInterpolate(p0, p1, r);
      }
      if (r >= pn.relative_current) {
        return InvertInterpolate(pn_1, pn, r);
      }
      for (std::size_t i = 0; i + 1u < count_; ++i) {
        const SensorResponsePoint& a = points_[i];
        const SensorResponsePoint& b = points_[i + 1u];
        if (r >= a.relative_current && r <= b.relative_current) {
          return InvertInterpolate(a, b, r);
        }
      }
    }

    return pn.distance_mm;
  }

 private:
  static float Clamp01(float x) noexcept {
    return std::clamp(x, 0.0f, 1.0f);
  }

  static float Interpolate(const SensorResponsePoint& a, const SensorResponsePoint& b,
                           float distance_mm) noexcept {
    const float dx = b.distance_mm - a.distance_mm;
    if (dx == 0.0f) {
      return a.relative_current;
    }
    const float t = (distance_mm - a.distance_mm) / dx;
    return a.relative_current + t * (b.relative_current - a.relative_current);
  }

  static float InvertInterpolate(const SensorResponsePoint& a, const SensorResponsePoint& b,
                                 float relative_current) noexcept {
    const float dr = b.relative_current - a.relative_current;
    if (dr == 0.0f) {
      return a.distance_mm;
    }
    const float t = (relative_current - a.relative_current) / dr;
    return a.distance_mm + t * (b.distance_mm - a.distance_mm);
  }

  const SensorResponsePoint* points_ = nullptr;
  std::size_t count_ = 0u;
};

}  // namespace midismith::sensor_linearization

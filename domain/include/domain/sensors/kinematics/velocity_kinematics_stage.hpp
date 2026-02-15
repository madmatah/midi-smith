#pragma once

namespace domain::sensors::kinematics {

template <float kDeltaD_mm, float kLRatio>
class VelocityKinematicsStage {
 public:
  void Reset() noexcept {}

  template <typename ContextT>
  float Transform(float speed_units_per_ms, const ContextT& ctx) noexcept {
    (void) ctx;
    return speed_units_per_ms * kDeltaD_mm * kLRatio;
  }
};

}  // namespace domain::sensors::kinematics

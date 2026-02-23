#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "piano-sensing/velocity/velocity_mapper_requirements.hpp"

namespace midismith::piano_sensing::velocity {

template <float kMaximumSpeedMPerS, float kShapeFactor>
class ExponentialVelocityMapper final : public VelocityMapperRequirements {
 public:
  static_assert(kMaximumSpeedMPerS > 0.0f,
                "kMaximumSpeedMPerS must be strictly positive for exponential velocity mapping.");
  static_assert(kShapeFactor > 0.0f,
                "kShapeFactor must be strictly positive for exponential velocity mapping.");

  midismith::midi::Velocity Map(float speed_m_per_s) noexcept override {
    if (!std::isfinite(speed_m_per_s) || speed_m_per_s <= 0.0f) {
      return static_cast<midismith::midi::Velocity>(127u);
    }

    const float normalized_speed = std::min(speed_m_per_s / kMaximumSpeedMPerS, 1.0f);
    const float mapped_velocity = 127.0f * std::pow(normalized_speed, kShapeFactor);

    if (!std::isfinite(mapped_velocity)) {
      return static_cast<midismith::midi::Velocity>(127u);
    }

    const auto rounded = static_cast<std::int32_t>(std::lround(mapped_velocity));
    const auto clamped = std::clamp<std::int32_t>(rounded, 1, 127);

    return static_cast<midismith::midi::Velocity>(static_cast<std::uint8_t>(clamped));
  }
};

}  // namespace midismith::piano_sensing::velocity

#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "domain/music/piano/velocity_mapper_requirements.hpp"

namespace domain::music::piano {

class GoeblLogarithmicVelocityMapper final : public VelocityMapperRequirements {
 public:
  domain::music::Velocity Map(float speed_m_per_s) noexcept override {
    if (speed_m_per_s <= 0.0f) {
      return static_cast<domain::music::Velocity>(1u);
    }

    const double midi_velocity = 57.96 + 71.3 * std::log10(static_cast<double>(speed_m_per_s));
    const auto rounded_long = std::lround(midi_velocity);
    const auto rounded = static_cast<std::int32_t>(rounded_long);
    const auto clamped = std::clamp<std::int32_t>(rounded, std::int32_t{1}, std::int32_t{127});
    return static_cast<domain::music::Velocity>(static_cast<std::uint8_t>(clamped));
  }
};

}  // namespace domain::music::piano

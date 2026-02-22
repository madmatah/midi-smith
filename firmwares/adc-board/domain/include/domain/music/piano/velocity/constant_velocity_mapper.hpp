#pragma once

#include <cstdint>

#include "domain/music/piano/velocity/velocity_mapper_requirements.hpp"

namespace midismith::adc_board::domain::music::piano::velocity {

template <std::uint8_t kFixedVelocity>
class ConstantVelocityMapper final : public VelocityMapperRequirements {
 public:
  static_assert(kFixedVelocity >= 1u && kFixedVelocity <= 127u,
                "kFixedVelocity must be in range [1, 127].");

  midismith::common::domain::music::Velocity Map(float speed_m_per_s) noexcept override {
    (void) speed_m_per_s;
    return kFixedVelocity;
  }
};

}  // namespace midismith::adc_board::domain::music::piano::velocity

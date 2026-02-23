#pragma once

#include <cstdint>

#include "piano-sensing/velocity/velocity_mapper_requirements.hpp"

namespace midismith::piano_sensing::velocity {

template <std::uint8_t kFixedVelocity>
class ConstantVelocityMapper final : public VelocityMapperRequirements {
 public:
  static_assert(kFixedVelocity >= 1u && kFixedVelocity <= 127u,
                "kFixedVelocity must be in range [1, 127].");

  midismith::midi::Velocity Map(float speed_m_per_s) noexcept override {
    (void) speed_m_per_s;
    return kFixedVelocity;
  }
};

}  // namespace midismith::piano_sensing::velocity

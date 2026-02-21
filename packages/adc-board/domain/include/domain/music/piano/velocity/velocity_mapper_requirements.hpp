#pragma once

#include "domain/music/types.hpp"

namespace domain::music::piano::velocity {

class VelocityMapperRequirements {
 public:
  virtual ~VelocityMapperRequirements() = default;

  virtual domain::music::Velocity Map(float speed_m_per_s) noexcept = 0;
};

}  // namespace domain::music::piano::velocity

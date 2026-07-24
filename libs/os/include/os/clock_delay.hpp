#pragma once

#include <cstdint>

#include "os-types/delay_requirements.hpp"

namespace midismith::os {

class ClockDelay final : public DelayRequirements {
 public:
  void DelayMs(std::uint32_t milliseconds) noexcept override;
};

}  // namespace midismith::os

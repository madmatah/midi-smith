#pragma once

#include "app/shell/blocking_delay_requirements.hpp"

namespace midismith::main_board::app::shell {

class OsClockBlockingDelay final : public BlockingDelayRequirements {
 public:
  void DelayMs(std::uint32_t delay_ms) noexcept override;
};

}  // namespace midismith::main_board::app::shell

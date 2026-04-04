#pragma once

#include "bsp-types/board_reset_requirements.hpp"

namespace midismith::bsp {

class Stm32BoardReset final : public BoardResetRequirements {
 public:
  void ResetBoard() noexcept override;
};

}  // namespace midismith::bsp

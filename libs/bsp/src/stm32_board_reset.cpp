#include "bsp/stm32_board_reset.hpp"

#include "stm32h7xx_hal.h"

namespace midismith::bsp {

void Stm32BoardReset::ResetBoard() noexcept {
  NVIC_SystemReset();
}

}  // namespace midismith::bsp

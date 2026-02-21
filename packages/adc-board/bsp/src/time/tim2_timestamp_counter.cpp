#include "bsp/time/tim2_timestamp_counter.hpp"

#include "stm32h7xx_hal.h"
#include "tim.h"

namespace bsp::time {
namespace {

void StartTim2() noexcept {
  (void) HAL_TIM_Base_Start(&htim2);
}

std::uint32_t NowTim2Ticks() noexcept {
  return __HAL_TIM_GET_COUNTER(&htim2);
}

}  // namespace

TimestampCounter CreateTim2TimestampCounter() noexcept {
  return TimestampCounter(StartTim2, NowTim2Ticks);
}

}  // namespace bsp::time

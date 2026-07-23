#include "bsp/rotary_encoder.hpp"

#include "stm32h7xx_hal.h"

namespace midismith::main_board::bsp {

namespace {

TIM_HandleTypeDef* TimerHandle(void* handle) noexcept {
  return reinterpret_cast<TIM_HandleTypeDef*>(handle);
}

}  // namespace

RotaryEncoder::RotaryEncoder(void* timer_handle) noexcept : timer_handle_(timer_handle) {}

void RotaryEncoder::Start() noexcept {
  HAL_TIM_Encoder_Start(TimerHandle(timer_handle_), TIM_CHANNEL_ALL);
  previous_counter_ = static_cast<std::uint16_t>(__HAL_TIM_GET_COUNTER(TimerHandle(timer_handle_)));
  pending_counts_ = 0;
}

std::int16_t RotaryEncoder::ReadDeltaDetents() noexcept {
  const auto current_counter =
      static_cast<std::uint16_t>(__HAL_TIM_GET_COUNTER(TimerHandle(timer_handle_)));
  const auto delta_counts = static_cast<std::int16_t>(current_counter - previous_counter_);
  previous_counter_ = current_counter;
  pending_counts_ = static_cast<std::int16_t>(pending_counts_ + delta_counts);
  const std::int16_t delta_detents = static_cast<std::int16_t>(pending_counts_ / kCountsPerDetent);
  pending_counts_ = static_cast<std::int16_t>(pending_counts_ % kCountsPerDetent);
  return delta_detents;
}

}  // namespace midismith::main_board::bsp

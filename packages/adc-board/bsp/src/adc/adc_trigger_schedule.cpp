#include "bsp/adc/adc_trigger_schedule.hpp"

#include <cstdint>

#include "stm32h7xx_hal.h"
#include "tim.h"

namespace bsp::adc {
namespace {

std::uint32_t Apb1TimerClockHz() noexcept {
  RCC_ClkInitTypeDef clkconfig{};
  std::uint32_t flash_latency = 0;
  HAL_RCC_GetClockConfig(&clkconfig, &flash_latency);

  const std::uint32_t pclk1_hz = HAL_RCC_GetPCLK1Freq();
  if (clkconfig.APB1CLKDivider == RCC_HCLK_DIV1) {
    return pclk1_hz;
  }
  return 2u * pclk1_hz;
}

struct TimerSettings {
  std::uint32_t prescaler = 0;
  std::uint32_t period = 0;
  std::uint32_t tick_hz = 0;
};

TimerSettings ComputeTimerSettings(std::uint32_t timer_clk_hz, std::uint32_t target_hz) noexcept {
  if (target_hz == 0u) {
    return TimerSettings{};
  }

  const std::uint32_t desired_tick_hz = 1'000'000u;
  std::uint32_t prescaler =
      (timer_clk_hz > desired_tick_hz) ? ((timer_clk_hz / desired_tick_hz) - 1u) : 0u;
  if (prescaler > 0xFFFFu) {
    prescaler = 0xFFFFu;
  }

  const std::uint32_t tick_hz = timer_clk_hz / (prescaler + 1u);
  std::uint32_t period = 0u;
  if (tick_hz >= target_hz) {
    period = (tick_hz / target_hz) - 1u;
  }
  if (period > 0xFFFFu) {
    period = 0xFFFFu;
  }
  return TimerSettings{prescaler, period, tick_hz};
}

bool ConfigureTim3Runtime(std::uint32_t trigger_hz, std::uint32_t cc4_phase_us) noexcept {
  const TimerSettings settings = ComputeTimerSettings(Apb1TimerClockHz(), trigger_hz);
  if (settings.tick_hz == 0u) {
    return false;
  }

  (void) HAL_TIM_Base_Stop(&htim3);
  (void) HAL_TIM_OC_Stop(&htim3, TIM_CHANNEL_4);

  htim3.Instance->PSC = settings.prescaler;
  htim3.Instance->ARR = settings.period;

  const std::uint32_t tick_per_us_raw = settings.tick_hz / 1'000'000u;
  const std::uint32_t tick_per_us = (tick_per_us_raw == 0u) ? 1u : tick_per_us_raw;
  const std::uint32_t period_ticks = settings.period + 1u;
  const std::uint32_t phase_ticks =
      (cc4_phase_us == 0u) ? (period_ticks / 2u) : (cc4_phase_us * tick_per_us);
  const std::uint32_t compare = (period_ticks == 0u) ? 0u : (phase_ticks % period_ticks);

  htim3.Instance->CCR4 = compare;
  htim3.Instance->EGR = TIM_EGR_UG;

  if (HAL_TIM_Base_Start(&htim3) != HAL_OK) {
    return false;
  }
  if (HAL_TIM_OC_Start(&htim3, TIM_CHANNEL_4) != HAL_OK) {
    return false;
  }

  return true;
}

bool ConfigureTim4Runtime(std::uint32_t trigger_hz, std::uint32_t update_phase_us) noexcept {
  const TimerSettings settings = ComputeTimerSettings(Apb1TimerClockHz(), trigger_hz);
  if (settings.tick_hz == 0u) {
    return false;
  }

  (void) HAL_TIM_Base_Stop(&htim4);

  htim4.Instance->PSC = settings.prescaler;
  htim4.Instance->ARR = settings.period;

  const std::uint32_t tick_per_us_raw = settings.tick_hz / 1'000'000u;
  const std::uint32_t tick_per_us = (tick_per_us_raw == 0u) ? 1u : tick_per_us_raw;
  const std::uint32_t modulo = settings.period + 1u;
  const std::uint32_t phase_ticks =
      (update_phase_us == 0u) ? (modulo / 2u) : (update_phase_us * tick_per_us);
  const std::uint32_t counter = (modulo == 0u) ? 0u : ((modulo - (phase_ticks % modulo)) % modulo);

  htim4.Instance->CNT = counter;
  htim4.Instance->EGR = TIM_EGR_UG;

  if (HAL_TIM_Base_Start(&htim4) != HAL_OK) {
    return false;
  }
  return true;
}

}  // namespace

bool AdcTriggerSchedule::Start(const AdcTriggerScheduleConfig& config) noexcept {
  if (running_) {
    Stop();
  }

  if (config.channel_rate_hz == 0u) {
    return false;
  }

  const std::uint32_t adc1_trigger_hz = 7u * config.channel_rate_hz;
  const std::uint32_t adc3_trigger_hz = 8u * config.channel_rate_hz;

  if (!ConfigureTim3Runtime(adc1_trigger_hz, config.adc2_phase_us)) {
    Stop();
    return false;
  }
  if (!ConfigureTim4Runtime(adc3_trigger_hz, config.adc3_phase_us)) {
    Stop();
    return false;
  }

  running_ = true;
  return true;
}

void AdcTriggerSchedule::Stop() noexcept {
  if (!running_) {
    return;
  }
  (void) HAL_TIM_OC_Stop(&htim3, TIM_CHANNEL_4);
  (void) HAL_TIM_Base_Stop(&htim3);
  (void) HAL_TIM_Base_Stop(&htim4);
  running_ = false;
}

}  // namespace bsp::adc

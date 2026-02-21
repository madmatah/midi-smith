#include "bsp/adc/adc_dma.hpp"
#include "stm32h7xx_hal.h"
#include "tim.h"

extern "C" void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef* hadc) {
  if (hadc == nullptr) {
    return;
  }

  if (hadc->Instance != ADC1 && hadc->Instance != ADC2 && hadc->Instance != ADC3) {
    return;
  }

  const std::uint32_t timestamp_ticks = __HAL_TIM_GET_COUNTER(&htim2);

  bsp::adc::AdcDma* adc_dma = bsp::adc::GetAdcDma();
  if (adc_dma == nullptr) {
    return;
  }

  if (hadc->Instance == ADC1) {
    adc_dma->HandleHalfComplete(bsp::adc::AdcGroup::kAdc1, timestamp_ticks);
    return;
  }

  if (hadc->Instance == ADC2) {
    adc_dma->HandleHalfComplete(bsp::adc::AdcGroup::kAdc2, timestamp_ticks);
    return;
  }

  if (hadc->Instance == ADC3) {
    adc_dma->HandleHalfComplete(bsp::adc::AdcGroup::kAdc3, timestamp_ticks);
  }
}

extern "C" void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc) {
  if (hadc == nullptr) {
    return;
  }

  if (hadc->Instance != ADC1 && hadc->Instance != ADC2 && hadc->Instance != ADC3) {
    return;
  }

  const std::uint32_t timestamp_ticks = __HAL_TIM_GET_COUNTER(&htim2);

  bsp::adc::AdcDma* adc_dma = bsp::adc::GetAdcDma();
  if (adc_dma == nullptr) {
    return;
  }

  if (hadc->Instance == ADC1) {
    adc_dma->HandleFullComplete(bsp::adc::AdcGroup::kAdc1, timestamp_ticks);
    return;
  }

  if (hadc->Instance == ADC2) {
    adc_dma->HandleFullComplete(bsp::adc::AdcGroup::kAdc2, timestamp_ticks);
    return;
  }

  if (hadc->Instance == ADC3) {
    adc_dma->HandleFullComplete(bsp::adc::AdcGroup::kAdc3, timestamp_ticks);
  }
}

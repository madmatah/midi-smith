#include "bsp/adc/adc_dma.hpp"

#include "SEGGER_RTT.h"
#include "adc.h"
#include "app/config/analog_acquisition.hpp"
#include "bsp/adc/adc_trigger_schedule.hpp"
#include "bsp/memory_sections.hpp"
#include "stm32h7xx_hal.h"

extern "C" DMA_HandleTypeDef hdma_adc1;
extern "C" DMA_HandleTypeDef hdma_adc2;

namespace bsp::adc {

namespace {

alignas(32) BSP_AXI_SRAM_NOCACHE
    static std::uint16_t g_adc1_dma_buffer[AdcDma::kMaxAdc1HalfwordsPerBuffer];
alignas(32) BSP_AXI_SRAM_NOCACHE
    static std::uint16_t g_adc2_dma_buffer[AdcDma::kMaxAdc2HalfwordsPerBuffer];
alignas(32) BSP_D3_SRAM_NOCACHE
    static std::uint16_t g_adc3_dma_buffer[AdcDma::kMaxAdc3HalfwordsPerBuffer];

static AdcDma* g_adc_dma = nullptr;

static bool PushDescriptor(os::Queue<AdcFrameDescriptor, 8>& queue, AdcGroup group,
                           std::uint8_t half, std::uint32_t sequence_id,
                           std::uint32_t timestamp_ticks, const void* data,
                           std::uint16_t element_count, std::uint8_t element_size_bytes) noexcept {
  const AdcFrameDescriptor desc{group, half,          sequence_id,       timestamp_ticks,
                                data,  element_count, element_size_bytes};
  return queue.SendFromIsr(desc);
}

std::uint16_t SequencesPerHalfBufferFromConfig() noexcept {
  constexpr std::uint32_t configured = ::app::config::ANALOG_ACQUISITION_SEQUENCES_PER_HALF_BUFFER;
  static_assert(configured >= 1u, "ANALOG_ACQUISITION_SEQUENCES_PER_HALF_BUFFER must be >= 1");
  static_assert(configured <= AdcDma::kMaxSequencesPerHalfBuffer,
                "ANALOG_ACQUISITION_SEQUENCES_PER_HALF_BUFFER is too large");
  return static_cast<std::uint16_t>(configured);
}

bool ConfigureAdc2Dma() noexcept {
  if (hadc2.DMA_Handle == nullptr) {
    hadc2.DMA_Handle = &hdma_adc2;
  }
  return (hadc2.DMA_Handle != nullptr);
}

bool IsAdcKernelClockWithinLimit() noexcept {
  const std::uint32_t adc_kernel_clock_hz = HAL_RCCEx_GetPeriphCLKFreq(RCC_PERIPHCLK_ADC);
  if (adc_kernel_clock_hz == 0u) {
    return false;
  }
  if (adc_kernel_clock_hz > ::app::config::ANALOG_ADC_KERNEL_CLOCK_LIMIT_HZ) {
    (void) SEGGER_RTT_printf(0u, "ADC kernel clock too high: %lu Hz (limit: %lu Hz)\r\n",
                             adc_kernel_clock_hz, ::app::config::ANALOG_ADC_KERNEL_CLOCK_LIMIT_HZ);
    return false;
  }
  return true;
}

bool CalibrateAdcsOnce() noexcept {
  static bool calibrated = false;
  if (calibrated) {
    return true;
  }

  if (HAL_ADCEx_Calibration_Start(&hadc1, ADC_CALIB_OFFSET_LINEARITY, ADC_SINGLE_ENDED) != HAL_OK) {
    return false;
  }
  if (HAL_ADCEx_Calibration_Start(&hadc2, ADC_CALIB_OFFSET_LINEARITY, ADC_SINGLE_ENDED) != HAL_OK) {
    return false;
  }
  if (HAL_ADCEx_Calibration_Start(&hadc3, ADC_CALIB_OFFSET_LINEARITY, ADC_SINGLE_ENDED) != HAL_OK) {
    return false;
  }

  calibrated = true;
  return true;
}

AdcTriggerSchedule& TriggerSchedule() noexcept {
  static AdcTriggerSchedule schedule;
  return schedule;
}

}  // namespace

AdcDma::AdcDma(os::Queue<AdcFrameDescriptor, 8>& queue) noexcept : queue_(queue) {
  RegisterAdcDma(*this);
}

bool AdcDma::Start() noexcept {
  Stop();

  if (!IsAdcKernelClockWithinLimit()) {
    Stop();
    return false;
  }

  if (!ConfigureAdc2Dma()) {
    Stop();
    return false;
  }


  if (!CalibrateAdcsOnce()) {
    Stop();
    return false;
  }

  const std::uint16_t sequences_per_half_buffer = SequencesPerHalfBufferFromConfig();
  (void) SEGGER_RTT_printf(
      0u, "ADC start: kernel=%lu Hz rate=%lu Hz seq_half=%u\r\n",
      static_cast<std::uint32_t>(HAL_RCCEx_GetPeriphCLKFreq(RCC_PERIPHCLK_ADC)),
      static_cast<std::uint32_t>(::app::config::ANALOG_ACQUISITION_CHANNEL_RATE_HZ),
      static_cast<unsigned>(sequences_per_half_buffer));

  adc1_halfwords_per_half_buffer_ =
      static_cast<std::uint16_t>(sequences_per_half_buffer * kAdc1RanksPerSequence);
  adc2_halfwords_per_half_buffer_ =
      static_cast<std::uint16_t>(sequences_per_half_buffer * kAdc2RanksPerSequence);
  adc3_halfwords_per_half_buffer_ =
      static_cast<std::uint16_t>(sequences_per_half_buffer * kAdc3RanksPerSequence);

  __disable_irq();
  running_ = true;
  __enable_irq();


  const std::uint32_t adc1_len = static_cast<std::uint32_t>(2u * adc1_halfwords_per_half_buffer_);
  const std::uint32_t adc2_len = static_cast<std::uint32_t>(2u * adc2_halfwords_per_half_buffer_);
  const std::uint32_t adc3_len = static_cast<std::uint32_t>(2u * adc3_halfwords_per_half_buffer_);

  if (HAL_ADC_Start_DMA(&hadc1, reinterpret_cast<std::uint32_t*>(g_adc1_dma_buffer), adc1_len) !=
      HAL_OK) {
    Stop();
    return false;
  }

  if (HAL_ADC_Start_DMA(&hadc2, reinterpret_cast<std::uint32_t*>(g_adc2_dma_buffer), adc2_len) !=
      HAL_OK) {
    Stop();
    return false;
  }

  if (HAL_ADC_Start_DMA(&hadc3, reinterpret_cast<std::uint32_t*>(g_adc3_dma_buffer), adc3_len) !=
      HAL_OK) {
    Stop();
    return false;
  }

  AdcTriggerScheduleConfig schedule_config{};
  schedule_config.channel_rate_hz = ::app::config::ANALOG_ACQUISITION_CHANNEL_RATE_HZ;
  schedule_config.adc2_phase_us = ::app::config::ANALOG_ADC2_PHASE_US;
  schedule_config.adc3_phase_us = ::app::config::ANALOG_ADC3_PHASE_US;

  if (!TriggerSchedule().Start(schedule_config)) {
    Stop();
    return false;
  }

  return true;
}

void AdcDma::Stop() noexcept {
  __disable_irq();
  running_ = false;
  __enable_irq();

  TriggerSchedule().Stop();

  (void) HAL_ADC_Stop_DMA(&hadc1);
  (void) HAL_ADC_Stop_DMA(&hadc2);
  (void) HAL_ADC_Stop_DMA(&hadc3);

  adc1_halfwords_per_half_buffer_ = 0;
  adc2_halfwords_per_half_buffer_ = 0;
  adc3_halfwords_per_half_buffer_ = 0;
}

void AdcDma::HandleHalfComplete(AdcGroup group, std::uint32_t timestamp_ticks) noexcept {
  if (!running_) {
    return;
  }

  if (group == AdcGroup::kAdc1) {
    adc1_sequence_id_++;
    (void) PushDescriptor(queue_, group, 0, adc1_sequence_id_, timestamp_ticks, g_adc1_dma_buffer,
                          adc1_halfwords_per_half_buffer_, sizeof(std::uint16_t));
    return;
  }

  if (group == AdcGroup::kAdc2) {
    adc2_sequence_id_++;
    (void) PushDescriptor(queue_, group, 0, adc2_sequence_id_, timestamp_ticks, g_adc2_dma_buffer,
                          adc2_halfwords_per_half_buffer_, sizeof(std::uint16_t));
    return;
  }

  if (group == AdcGroup::kAdc3) {
    adc3_sequence_id_++;
    (void) PushDescriptor(queue_, group, 0, adc3_sequence_id_, timestamp_ticks, g_adc3_dma_buffer,
                          adc3_halfwords_per_half_buffer_, sizeof(std::uint16_t));
  }
}

void AdcDma::HandleFullComplete(AdcGroup group, std::uint32_t timestamp_ticks) noexcept {
  if (!running_) {
    return;
  }

  if (group == AdcGroup::kAdc1) {
    adc1_sequence_id_++;
    (void) PushDescriptor(queue_, group, 1, adc1_sequence_id_, timestamp_ticks,
                          &g_adc1_dma_buffer[adc1_halfwords_per_half_buffer_],
                          adc1_halfwords_per_half_buffer_, sizeof(std::uint16_t));
    return;
  }

  if (group == AdcGroup::kAdc2) {
    adc2_sequence_id_++;
    (void) PushDescriptor(queue_, group, 1, adc2_sequence_id_, timestamp_ticks,
                          &g_adc2_dma_buffer[adc2_halfwords_per_half_buffer_],
                          adc2_halfwords_per_half_buffer_, sizeof(std::uint16_t));
    return;
  }

  if (group == AdcGroup::kAdc3) {
    adc3_sequence_id_++;
    (void) PushDescriptor(queue_, group, 1, adc3_sequence_id_, timestamp_ticks,
                          &g_adc3_dma_buffer[adc3_halfwords_per_half_buffer_],
                          adc3_halfwords_per_half_buffer_, sizeof(std::uint16_t));
  }
}

void RegisterAdcDma(AdcDma& adc_dma) noexcept {
  __disable_irq();
  g_adc_dma = &adc_dma;
  __enable_irq();
}

AdcDma* GetAdcDma() noexcept {
  return g_adc_dma;
}

}  // namespace bsp::adc

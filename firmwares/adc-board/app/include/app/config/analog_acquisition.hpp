#pragma once

#include <cstdint>

namespace midismith::adc_board::app::config {

// Analog acquisition
//
// Terminology:
// - "channel rate" means the target update rate per sensor (SEN1..SEN22).
// - ADCs run in triggered + discontinuous mode (1 conversion per trigger), so the trigger rate is:
//   - ADC1 trigger rate = 7 * ANALOG_ACQUISITION_CHANNEL_RATE_HZ
//   - ADC2 trigger rate = 7 * ANALOG_ACQUISITION_CHANNEL_RATE_HZ (phase-shifted from ADC1)
//   - ADC3 trigger rate = 8 * ANALOG_ACQUISITION_CHANNEL_RATE_HZ (phase-shifted)
// - DMA runs in circular mode and interrupts on half/full buffer completion only.
//   ANALOG_ACQUISITION_SEQUENCES_PER_HALF_BUFFER controls the trade-off:
//   - higher value: fewer DMA IRQs, higher latency
//   - lower value: more DMA IRQs, lower latency


// Target acquisition rate per channel
constexpr std::uint32_t ANALOG_ACQUISITION_CHANNEL_RATE_HZ = 18000;

// ADC kernel clock limit
// According to AN5354, STM32H743 using 3 ADC simultaneously at 16-bit, with
// package LQFP144, must use 7 MHz for ADC kernel clock.
// After testing, we can go up to 11.4 MHz without issues.
constexpr std::uint32_t ANALOG_ADC_KERNEL_CLOCK_LIMIT_HZ = 12'000'000;

// Controls the DMA interrupt frequency and system responsiveness.
// Each 'sequence' contains one measurement of all channels (7 or 8 ranks).
// - 1: Maximum reactivity. An IRQ is triggered as soon as a full scan is ready.
//      (e.g. ~3000 IRQ/s at 1kHz).
// - Higher: Groups multiple scans before notifying the CPU.
//      Reduces CPU overhead by processing data in larger batches, but adds latency.
constexpr std::uint32_t ANALOG_ACQUISITION_SEQUENCES_PER_HALF_BUFFER = 8;

// Phase shifts applied to the trigger schedule.
//
// Units: microseconds, expressed inside each ADC trigger period.
// - ADC2 is triggered by TIM3 Channel 4 compare event, relative to ADC1 TIM3 update event.
// - ADC3 is triggered by TIM4 update event, with an initial counter offset.
//
// A value of 0 means "use half-period" (auto 180° phase shift).
constexpr std::uint32_t ANALOG_ADC2_PHASE_US = 0;
constexpr std::uint32_t ANALOG_ADC3_PHASE_US = 0;

// Analog acquisition timestamping.
//
// TIM2 is used as a free-running 32-bit timebase at 1 MHz (1 tick = 1 µs).
// DMA callbacks sample TIM2 to timestamp each half-buffer.
//
// The acquisition task interpolates per-sequence timestamps inside a half-buffer.
// The first half-buffer after enabling has only one timestamp, so we use these estimates as a
// fallback until a second timestamp is available.
static_assert(ANALOG_ACQUISITION_CHANNEL_RATE_HZ > 0u,
              "ANALOG_ACQUISITION_CHANNEL_RATE_HZ must be > 0");
constexpr std::uint32_t ANALOG_TICKS_PER_SECOND = 1'000'000u;

constexpr std::uint32_t ANALOG_TICKS_PER_SEQUENCE_ESTIMATE =
    (ANALOG_TICKS_PER_SECOND + (ANALOG_ACQUISITION_CHANNEL_RATE_HZ / 2u)) /
    ANALOG_ACQUISITION_CHANNEL_RATE_HZ;
constexpr std::uint32_t ANALOG_ADC12_TICKS_PER_SEQUENCE_ESTIMATE =
    ANALOG_TICKS_PER_SEQUENCE_ESTIMATE;
constexpr std::uint32_t ANALOG_ADC3_TICKS_PER_SEQUENCE_ESTIMATE =
    ANALOG_TICKS_PER_SEQUENCE_ESTIMATE;

}  // namespace midismith::adc_board::app::config

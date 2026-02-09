#pragma once

#include <cstdint>

#include "domain/dsp/concepts.hpp"

namespace domain::dsp::converters {

/**
 * @brief Converts raw ADC counts from a TIA stage into physical current (mA).
 * Formula: I_in = (AdcMaxValue - ADC_val) * (Vref_mV / (AdcMaxValue * Rf_Ohms))
 * @tparam kVrefMilliVolts The voltage reference in mV (e.g., 2048)
 * @tparam kAdcBits The resolution of the ADC (e.g., 16)
 * @tparam kRfOhms The feedback resistor value in Ohms (e.g., 1800)
 */
template <std::int32_t kVrefMilliVolts, std::int32_t kAdcBits, std::int32_t kRfOhms>
class TiaCurrentConverter {
  static_assert(kAdcBits > 0 && kAdcBits <= 32, "ADC bits must be between 1 and 32");
  static_assert(kRfOhms > 0, "Feedback resistor cannot be zero");

  static constexpr float kAdcMaxValue = static_cast<float>((1ULL << kAdcBits) - 1);
  static constexpr float kScaleFactor =
      static_cast<float>(kVrefMilliVolts) / (kAdcMaxValue * static_cast<float>(kRfOhms));

 public:
  void Reset() noexcept {}

  template <typename ContextT>
  float Transform(float raw_adc_counts, const ContextT& ctx) noexcept {
    (void) ctx;
    return (kAdcMaxValue - raw_adc_counts) * kScaleFactor;
  }
};

}  // namespace domain::dsp::converters

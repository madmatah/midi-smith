#pragma once

namespace midismith::adc_board::domain::dsp::logic {

template <float kThreshold, auto ValueProvider>
struct GateOpen {
  template <typename ContextT>
  static inline bool Test(const ContextT& ctx) noexcept {
    return static_cast<float>(ValueProvider(ctx)) < kThreshold;
  }
};

}  // namespace midismith::adc_board::domain::dsp::logic

#pragma once

namespace midismith::adc_board::domain::dsp::logic {

template <auto ValueProvider>
struct IsFalse {
  template <typename ContextT>
  static inline bool Test(const ContextT& ctx) noexcept {
    return static_cast<bool>(ValueProvider(ctx)) == false;
  }
};

}  // namespace midismith::adc_board::domain::dsp::logic

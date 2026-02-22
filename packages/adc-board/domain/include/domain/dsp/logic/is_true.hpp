#pragma once

namespace midismith::adc_board::domain::dsp::logic {

template <auto ValueProvider>
struct IsTrue {
  template <typename ContextT>
  static inline bool Test(const ContextT& ctx) noexcept {
    return static_cast<bool>(ValueProvider(ctx)) == true;
  }
};

}  // namespace midismith::adc_board::domain::dsp::logic

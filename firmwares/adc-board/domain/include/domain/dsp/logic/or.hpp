#pragma once

namespace midismith::adc_board::domain::dsp::logic {

template <typename... Predicates>
struct Or {
  template <typename ContextT>
  static inline bool Test(const ContextT& ctx) noexcept {
    return (Predicates::Test(ctx) || ...);
  }
};

}  // namespace midismith::adc_board::domain::dsp::logic

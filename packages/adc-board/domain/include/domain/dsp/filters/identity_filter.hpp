#pragma once

namespace midismith::adc_board::domain::dsp::filters {

class IdentityFilter {
 public:
  template <typename ContextT>
  float Transform(float sample, const ContextT& ctx) noexcept {
    (void) ctx;
    return sample;
  }
  void Reset() noexcept {}
};

}  // namespace midismith::adc_board::domain::dsp::filters

#pragma once

namespace midismith::dsp::filters {

template <float kValue>
class ConstantFilter {
 public:
  template <typename ContextT>
  inline float Transform(float sample, const ContextT& ctx) noexcept {
    (void) sample;
    (void) ctx;
    return kValue;
  }

  inline void Reset() noexcept {}
};

}  // namespace midismith::dsp::filters

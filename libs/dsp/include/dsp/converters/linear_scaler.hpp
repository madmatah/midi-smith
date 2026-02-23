#pragma once

namespace midismith::dsp::converters {

template <float kScale>
class LinearScaler {
 public:
  void Reset() noexcept {}

  template <typename ContextT>
  float Transform(float input, const ContextT& ctx) noexcept {
    (void) ctx;
    return input * kScale;
  }
};

}  // namespace midismith::dsp::converters

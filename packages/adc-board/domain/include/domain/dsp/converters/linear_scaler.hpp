#pragma once

namespace domain::dsp::converters {

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

}  // namespace domain::dsp::converters

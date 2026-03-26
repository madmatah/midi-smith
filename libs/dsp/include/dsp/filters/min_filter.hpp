#pragma once

#include <limits>

namespace midismith::dsp::filters {

class MinFilter {
 public:
  void Reset() noexcept {
    min_ = std::numeric_limits<float>::max();
  }

  template <typename ContextT>
  float Transform(float sample, const ContextT& /*ctx*/) noexcept {
    if (sample < min_) min_ = sample;
    return min_;
  }

 private:
  float min_ = std::numeric_limits<float>::max();
};

}  // namespace midismith::dsp::filters

#pragma once

#include <limits>

namespace midismith::dsp::filters {

class MaxFilter {
 public:
  void Reset() noexcept {
    max_ = std::numeric_limits<float>::lowest();
  }

  template <typename ContextT>
  float Transform(float sample, const ContextT& /*ctx*/) noexcept {
    if (sample > max_) max_ = sample;
    return max_;
  }

 private:
  float max_ = std::numeric_limits<float>::lowest();
};

}  // namespace midismith::dsp::filters
